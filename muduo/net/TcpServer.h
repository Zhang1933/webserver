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
class EventLoopThreadPool;

class TcpServer: noncopyable{
public:
    typedef std::function<void(EventLoop*)> ThreadInitCallback;

    TcpServer(EventLoop *loop,const InetAddress& listenAddr);
    ~TcpServer(); // force out-line dtor, for scoped_ptr members.

    EventLoop* getLoop() const { return loop_; }
    const string& name() const { return name_; }
    const string& ipPort() const { return name_; } //TODO： name与ipPort分开

    /// Set the number of threads for handling input.
    ///
    /// Always accepts new connection in loop's thread.
    /// Must be called before @c start
    /// @param numThreads
    /// - 0 means all I/O in loop's thread, no thread will created.
    ///   this is the default value.
    /// - 1 means all I/O in another thread.
    /// - N means a thread pool with N threads, new connections
    ///   are assigned on a round-robin basis.
    void setThreadNum(int numThreads);
    void setThreadInitCallback(const ThreadInitCallback& cb)
    { threadInitCallback_ = cb; }
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
    /// Thread safe.
    void removeConnection(const TcpConnectionPtr& conn);
    /// Not thread safe, but in loop
    void removeConnectionInLoop(const TcpConnectionPtr& conn);

    EventLoop* loop_; // the acceptor loop
    const std::string name_;
    bool started_;
    std::unique_ptr<Acceptor>acceptor_; // avoid revealing Acceptor
    std::unique_ptr<EventLoopThreadPool> threadPool_;

    ThreadInitCallback threadInitCallback_;
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    int nextConnId_;// always in loop thread
    ConnectionMap connections_;
};
}

#endif