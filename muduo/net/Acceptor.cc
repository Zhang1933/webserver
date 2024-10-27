
#include "Acceptor.h"
#include "muduo/base/Logging.h"
#include "muduo/net/InetAddress.h"
#include "muduo/net/SocketsOps.h"
#include <functional>

using namespace muduo;

Acceptor::Acceptor(EventLoop* loop,const InetAddress& listenAddr)
    :loop_(loop),
    acceptSocket_(sockets::createNonblockingOrDie()),
    acceptChannel_(loop,acceptSocket_.fd()),
    listenning_(false)
{
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.bindAddress(listenAddr);
    acceptChannel_.setReadCallback(
        std::bind(&Acceptor::handleRead,this));
}

void Acceptor::listen()
{
    loop_->assertInLoopThread();
    listenning_=true;
    acceptSocket_.listen();
    acceptChannel_.enableReading();
}

void Acceptor::handleRead()
{
    loop_->assertInLoopThread();
    InetAddress peerAddr(0);
      //FIXME loop until no more
    int connfd=acceptSocket_.accept(&peerAddr);
    if(connfd>=0)
    {
        if(newConnectionCallback_){
            newConnectionCallback_(connfd,peerAddr);
        }
        else{
            LOG_INFO<<"acceptor does not have call back,close socket";
            sockets::close(connfd);
        }
    }
    else {
        LOG_WARN<<"Acceptor::handleRead connfd<0: "<<connfd;
    }
}