#ifndef MUDUO_NET_ACCEPTOR_H
#define MUDUO_NET_ACCEPTOR_H

#include "muduo/base/noncopyable.h"
#include "muduo/net/Channel.h"
#include"muduo/net/Socket.h"

#include <functional>
namespace muduo {

class EventLoop;
class InetAddress;

class Acceptor:noncopyable
{
public:
    typedef std::function<void(int sockfd,const InetAddress&)> NewConnectionCallback;
    Acceptor(EventLoop* loop,const InetAddress& listenAddr);
    void setNewConnectionCallback(const NewConnectionCallback&cb)
    {newConnectionCallback_=cb;}

    bool listening()const{return listenning_;}
    void listen();

private:
    void handleRead();
    EventLoop* loop_;
    Socket acceptSocket_;
    Channel acceptChannel_;
    NewConnectionCallback newConnectionCallback_;
    bool listenning_;
};
}

#endif