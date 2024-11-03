#include "muduo/net/TcpConnection.h"
#include "muduo/base/Logging.h"
#include "muduo/base/Types.h"
#include "muduo/net/Channel.h"
#include "muduo/net/Socket.h"
#include "muduo/net/SocketsOps.h"
#include <cassert>
#include <cstdlib>
#include <functional>
#include <string>
#include <sys/types.h>
#include <unistd.h>

using namespace muduo;


void muduo::defaultConnectionCallback(const TcpConnectionPtr& conn)
{
  LOG_TRACE << conn->localAddress().toHostPort() << " -> "
            << conn->peerAddress().toHostPort() << " is "
            << (conn->connected() ? "UP" : "DOWN");
  // do not call conn->forceClose(), because some users want to register message callback only.
}

void muduo::defaultMessageCallback(const TcpConnectionPtr&,
                                        Buffer* buf,
                                        Timestamp)
{
  buf->retrieveAll();
}

TcpConnection::TcpConnection(EventLoop *loop,const std::string&nameArg,
                int sockfd,
                const InetAddress& localAddr,
                const InetAddress& peerAddr)
                :loop_(CHECK_NOTNULL(loop)),
                name_(nameArg),
                state_(kConnecting),
                socket_(new Socket(sockfd)),
                channel_(new Channel(loop,sockfd)),
                localAddr_(localAddr),
                peerAddr_(peerAddr)
{
    LOG_DEBUG << "TcpConnection::ctor[" <<  name_ << "] at " << this
            << " fd=" << sockfd;
    channel_->setReadCallback(
      std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
    channel_->setWriteCallback(
      std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(
      std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(
      std::bind(&TcpConnection::handleError, this));
    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
      LOG_DEBUG << "TcpConnection::dtor[" <<  name_ << "] at " << this
            << " fd=" << channel_->fd();
}

void TcpConnection::handleRead(Timestamp receiveTime)
{
    loop_->assertInLoopThread();
    int savedErrno=0;
    ssize_t n=inputBuffer_.readFd(channel_->fd(), &savedErrno);
    if (n > 0) {
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    } else if (n == 0) {
        handleClose();
    } else {
        errno = savedErrno;
        LOG_SYSERR << "TcpConnection::handleRead";
        handleError();
  }
}

void TcpConnection::handleClose()
{
    loop_->assertInLoopThread();
    LOG_TRACE << "fd = " << channel_->fd() << " state = " << stateToString();
    assert(state_ == kConnected|| state_ == kDisconnecting);
    // we don't close fd, leave it to dtor, so we can find leaks easily.
    channel_->disableAll();
    // must be the last line。TcpConnection::handleClose() 的主要功能是调用 closeCallback_,这个回调  绑定到 TcpServer::removeConnection()
    closeCallback_(shared_from_this());
}

void TcpConnection::handleError()
{
    // TODO: 没有进一步的行动,只是在日志中输出错误消息,为什么不影响连接的正常关闭
  int err = sockets::getSocketError(channel_->fd());
  LOG_ERROR << "TcpConnection::handleError [" << name_
            << "] - SO_ERROR = " << err << " " << strerror_tl(err);
}


void TcpConnection::connectEstablished()
{
    loop_->assertInLoopThread();
    assert(state_==kConnecting);
    setState(kConnected);
    channel_->enableReading();
    connectionCallback_(shared_from_this());
}

void TcpConnection::connectDestroyed()
{
  loop_->assertInLoopThread();
  assert(state_ == kConnected || state_ == kDisconnecting);
  setState(kDisconnected);
  channel_->disableAll();
  connectionCallback_(shared_from_this());

  loop_->removeChannel(channel_.get());
}

void TcpConnection::shutdown()
{
  // FIXME: use compare and swap
  if (state_ == kConnected)
  {
    setState(kDisconnecting);
    // FIXME: shared_from_this()?
    loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
  }
}

void TcpConnection::shutdownInLoop()
{
    loop_->assertInLoopThread();
    if(!channel_->isWriting())
    {
        // we are not writing
        socket_->shutdownWrite();
    }
}

void TcpConnection::setTcpNoDelay(bool on)
{
  socket_->setTcpNoDelay(on);
}

void TcpConnection::handleWrite()
{
  loop_->assertInLoopThread();
  if (channel_->isWriting()) {
    ssize_t n = ::write(channel_->fd(),
                        outputBuffer_.peek(),
                        outputBuffer_.readableBytes());
    if (n > 0) {
      outputBuffer_.retrieve(static_cast<size_t>(n));
      if (outputBuffer_.readableBytes() == 0) {
        channel_->disableWriting();
        if (writeCompleteCallback_) {
          loop_->queueInLoop(
              std::bind(writeCompleteCallback_, shared_from_this()));
        }
        if (state_ == kDisconnecting) {
          LOG_INFO<<"write complete, state:"<<state_<<"shutdown con";
          shutdownInLoop();
        }
      } else {
        LOG_TRACE << "I am going to write more data";
      }
    } else {
      LOG_SYSERR << "TcpConnection::handleWrite";
    }
  } else {
    LOG_TRACE << "Connection is down, no more writing";
  }
}

void TcpConnection::send(const void* data, size_t len)
{
  send(std::string(static_cast<const char*>(data), len));
}


void TcpConnection::send(const std::string& message)
{
  if (state_ == kConnected) {
    if (loop_->isInLoopThread()) {
      sendInLoop(message);
    } else {
      loop_->runInLoop(
          std::bind(&TcpConnection::sendInLoop, this,message));
    }
  }
}

void TcpConnection::send(Buffer* buf)
{
  if (state_ == kConnected)
  {
    if (loop_->isInLoopThread())
    {
      sendInLoop(buf->retrieveAsString());
      buf->retrieveAll();
    }
    else
    {
      void (TcpConnection::*fp)(const std::string& message) = &TcpConnection::sendInLoop;
      loop_->runInLoop(
          std::bind(fp,
                    this,     // FIXME
                    buf->retrieveAsString()));
                    //std::forward<string>(message)));
    }
  }
}

void TcpConnection::sendInLoop(const std::string& message)
{
  loop_->assertInLoopThread();
  ssize_t nwrote = 0;
  // if no thing in output queue, try writing directly
  if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0) {
    nwrote = ::write(channel_->fd(), message.data(), message.size());
    if (nwrote >= 0) {
      if (implicit_cast<size_t>(nwrote) < message.size()) {
        LOG_TRACE << "I am going to write more data";
      } else if (writeCompleteCallback_) {
        loop_->queueInLoop(
            std::bind(writeCompleteCallback_, shared_from_this()));
      }
    } else {
      nwrote = 0;
      if (errno != EWOULDBLOCK) {
        LOG_SYSERR << "TcpConnection::sendInLoop";
      }
    }
  }

  assert(nwrote >= 0);
  if (implicit_cast<size_t>(nwrote) < message.size()) {
    outputBuffer_.append(message.data()+nwrote, message.size()-static_cast<size_t>(nwrote));
    if (!channel_->isWriting()) {
      channel_->enableWriting();
    }
  }
}

const char* TcpConnection::stateToString() const
{
  switch (state_)
  {
    case kDisconnected:
      return "kDisconnected";
    case kConnecting:
      return "kConnecting";
    case kConnected:
      return "kConnected";
    case kDisconnecting:
      return "kDisconnecting";
    default:
      return "unknown state";
  }
}