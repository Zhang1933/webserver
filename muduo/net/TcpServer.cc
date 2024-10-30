#include "muduo/net/TcpServer.h"
#include "muduo/base/Logging.h"
#include "muduo/net/Acceptor.h"
#include "muduo/net/Callback.h"
#include "muduo/net/EventLoop.h"
#include "muduo/net/InetAddress.h"
#include "muduo/net/TcpConnection.h"
#include <cassert>
#include <cstddef>
#include <functional>
#include <string>
#include"muduo/net/SocketsOps.h"


using namespace muduo;

TcpServer::TcpServer(EventLoop *loop,const InetAddress& listenAddr)
    :loop_(CHECK_NOTNULL(loop)),
    name_(listenAddr.toHostPort()),
    started_(false),
    acceptor_(new Acceptor(loop,listenAddr)),
    nextConnId_(1)
{
    acceptor_->setNewConnectionCallback(
        std::bind(&TcpServer::newConnection,this,std::placeholders::_1,std::placeholders::_2));
}
TcpServer::~TcpServer()
{
}

void TcpServer::start()
{
    if(!started_)
    {
        started_=true;
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
  char buf[32];
  snprintf(buf, sizeof buf, "#%d", nextConnId_);
  ++nextConnId_;
  std::string connName = name_ + buf;

  LOG_INFO << "TcpServer::newConnection [" << name_
           << "] - new connection [" << connName
           << "] from " << peerAddr.toHostPort();
  InetAddress localAddr(sockets::getLocalAddr(sockfd));
  // FIXME poll with zero timeout to double confirm the new connection
  TcpConnectionPtr conn(
      new TcpConnection(loop_, connName, sockfd, localAddr, peerAddr));
  connections_[connName] = conn;
  conn->setConnectionCallback(connectionCallback_);
  conn->setMessageCallback(messageCallback_);
  conn->setWriteCompleteCallback(writeCompleteCallback_);
  conn->setCloseCallback(
      std::bind(&TcpServer::removeConnection, this, std::placeholders::_1)); // FIXME: unsafe
  conn->connectEstablished();
}

void TcpServer::removeConnection(const TcpConnectionPtr& conn)
{
    loop_->assertInLoopThread();
    LOG_INFO << "TcpServer::removeConnection [" << name_
        << "] - connection " << conn->name();
    size_t n=connections_.erase(conn->name());
    assert(n==1);(void)n;
    loop_->queueInLoop(
      std::bind(&TcpConnection::connectDestroyed, conn));
}