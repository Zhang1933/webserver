#include "muduo/net/TcpConnection.h"
#include "muduo/base/Logging.h"
#include "muduo/net/Channel.h"
#include "muduo/net/Socket.h"
#include "muduo/net/SocketsOps.h"
#include <cassert>
#include <cstdlib>
#include <functional>
#include <sys/types.h>
#include <unistd.h>

using namespace muduo;

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
        std::bind(&TcpConnection::handleRead,this));
          std::bind(&TcpConnection::handleWrite, this);
    channel_->setCloseCallback(
      std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(
      std::bind(&TcpConnection::handleError, this));
}

TcpConnection::~TcpConnection()
{
      LOG_DEBUG << "TcpConnection::dtor[" <<  name_ << "] at " << this
            << " fd=" << channel_->fd();
}

void TcpConnection::handleRead()
{
    char buf[65536];
    ssize_t n=::read(channel_->fd(), buf, sizeof buf);
    if (n > 0) {
    messageCallback_(shared_from_this(), buf, n);
    } else if (n == 0) {
        handleClose();
    } else {
        handleError();
  }

}

void TcpConnection::handleWrite()
{
}

void TcpConnection::handleClose()
{
    loop_->assertInLoopThread();
    LOG_TRACE << "TcpConnection::handleClose state = " << state_;
    assert(state_ == kConnected);
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
  assert(state_ == kConnected);
  setState(kDisconnected);
  channel_->disableAll();
  connectionCallback_(shared_from_this());

  loop_->removeChannel(channel_.get());
}