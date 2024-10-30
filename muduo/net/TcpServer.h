#ifndef MUDUO_NET_TCPSERVER_H
#define MUDUO_NET_TCPSERVER_H

#include "muduo/net/Callback.h"
#include "muduo/net/InetAddress.h"
#include "muduo/net/TcpConnection.h"
#include <map>
#include <memory>
#include <string>
namespace muduo {

class EventLoop;
class Acceptor;

class TcpServer{
public:
    TcpServer(EventLoop *loop,const InetAddress& listenAddr);
    ~TcpServer(); // force out-line dtor, for scoped_ptr members.

    /// Starts the server if it's not listenning.
    ///
    /// It's harmless to call it multiple times.
    /// Thread safe.
    void start();

    
  /// Set connection callback.
  /// Not thread safe.
  void setConnectionCallback(const ConnectionCallback& cb)
  { connectionCallback_ = cb; }

    /// Set message callback.
  /// Not thread safe.
  void setMessageCallback(const MessageCallback& cb)
  { messageCallback_ = cb; }

  /// Set write complete callback.
  /// Not thread safe.
  void setWriteCompleteCallback(const WriteCompleteCallback& cb)
  { writeCompleteCallback_ = cb; }


private:
    typedef std::map<std::string, TcpConnectionPtr> ConnectionMap;
  /// Not thread safe, but in loop
    void newConnection(int sockfd,const InetAddress&peerAddr);
    void removeConnection(const TcpConnectionPtr& conn); 
    EventLoop* loop_; // the acceptor loop
    const std::string name_;
    bool started_;
    std::unique_ptr<Acceptor>acceptor_; // avoid revealing Acceptor
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    int nextConnId_;// always in loop thread
    ConnectionMap connections_;
};
}

#endif