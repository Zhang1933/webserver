#include "muduo/net/TcpServer.h"
#include "muduo/base/Logging.h"
#include "muduo/net/Acceptor.h"
#include "muduo/net/Callback.h"
#include "muduo/net/EventLoop.h"
#include "muduo/net/InetAddress.h"
#include "muduo/net/TcpConnection.h"
#include "muduo/net/EventLoopThreadPool.h"
#include <cassert>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include"muduo/net/SocketsOps.h"


using namespace muduo;

TcpServer::TcpServer(EventLoop *loop,const InetAddress& listenAddr)
    :loop_(CHECK_NOTNULL(loop)),
    name_(listenAddr.toHostPort()),
    started_(false),
    acceptor_(new Acceptor(loop,listenAddr)),
    threadPool_(new EventLoopThreadPool(loop)),
    nextConnId_(1)
{
    acceptor_->setNewConnectionCallback(
        std::bind(&TcpServer::newConnection,this,std::placeholders::_1,std::placeholders::_2));
}
TcpServer::~TcpServer()
{
  loop_->assertInLoopThread();
  LOG_TRACE << "TcpServer::~TcpServer [" << name_ << "] destructing";

  for (auto& item : connections_)
  {
    TcpConnectionPtr conn(item.second);
    item.second.reset();
    conn->getLoop()->runInLoop(
      std::bind(&TcpConnection::connectDestroyed, conn));
  }
}

void TcpServer::setThreadNum(int numThreads)
{
  assert(0 <= numThreads);
  threadPool_->setThreadNum(numThreads);
}


void TcpServer::start()
{
    if(!started_)
    {
        started_=true;
        threadPool_->start(threadInitCallback_);
    }
    if(!acceptor_->listening())
    {
        loop_->runInLoop(
            std::bind(&Acceptor::listen,acceptor_.get()));
    }
}

void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr)
{
  loop_->assertInLoopThread();
  char buf[64];
  snprintf(buf, sizeof buf, "#%d-%s", nextConnId_,peerAddr.toHostPort().c_str());
  ++nextConnId_;
  std::string connName = name_ + buf;

  LOG_INFO << "TcpServer::newConnection [" << name_
           << "] - new connection [" << connName
           << "] from " << peerAddr.toHostPort();
  InetAddress localAddr(sockets::getLocalAddr(sockfd));
  // FIXME poll with zero timeout to double confirm the new connection
  // FIXME use make_shared if necessary
  EventLoop* ioLoop = threadPool_->getNextLoop();
  TcpConnectionPtr conn(
      std::make_shared<TcpConnection>(ioLoop, connName, sockfd, localAddr, peerAddr));
  connections_[connName] = conn;
  conn->setConnectionCallback(connectionCallback_);
  conn->setMessageCallback(messageCallback_);
  conn->setWriteCompleteCallback(writeCompleteCallback_);
  conn->setCloseCallback(
      std::bind(&TcpServer::removeConnection, this,_1)); // TODO:不调用TcpServer::removeConnection
  ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr& conn)
{
  // FIXME: unsafe
  loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn)
{
  loop_->assertInLoopThread();
  LOG_INFO << "TcpServer::removeConnectionInLoop [" << name_
           << "] - connection " << conn->name();
  size_t n = connections_.erase(conn->name());
  assert(n == 1); (void)n;
  EventLoop* ioLoop = conn->getLoop();
  ioLoop->queueInLoop(
      std::bind(&TcpConnection::connectDestroyed, conn));
}