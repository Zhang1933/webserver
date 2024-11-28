#ifndef MUDUO_NET_TCPCONNECTION_H
#define MUDUO_NET_TCPCONNECTION_H

#include "muduo/base/noncopyable.h"
#include "muduo/net/Callback.h"
#include "muduo/net/InetAddress.h"
#include "muduo/net/Buffer.h"
#include <cstddef>
#include <memory>
#include <string>
#include <boost/any.hpp>
#include <utility>

namespace muduo {

class EventLoop;
class Channel;
class Socket;

///
/// TCP connection, for both client and server usage.
///
class TcpConnection:noncopyable,
    public std::enable_shared_from_this<TcpConnection>
{
public:
  typedef std::pair<int, ssize_t> fileDesoff; // 文件名描述符与读的offset
  typedef std::pair<fileDesoff, bool> filectxPii;//第一个参数为文件名与offset pair，第二个参数为如果文件发送完毕，是否关闭连接。

  /// Constructs a TcpConnection with a connected sockfd
  ///
  /// User should not create this object.
    TcpConnection(EventLoop *loop,const std::string&name,
                int sockfd,
                const InetAddress& localAddr,
                const InetAddress& peerAddr);
    ~TcpConnection();

  EventLoop* getLoop() const { return loop_; }
  bool connected() const { return state_ == kConnected; }
  const std::string& name() const { return name_; }
  const InetAddress& localAddress() { return localAddr_; }
  const InetAddress& peerAddress() { return peerAddr_; }

    void send(const void* message, size_t len);
    // Thread safe.
    void send(const std::string& message);
    // Thread safe.
    void send(Buffer* message);  // this one will swap data
    void shutdown();
    void shutdownInLoop();

    void setTcpNoDelay(bool on);

    void setConnectionCallback(const ConnectionCallback& cb)
    {connectionCallback_=cb;}
    void setMessageCallback(const MessageCallback& cb)
    {messageCallback_=cb;}

    void setWriteCompleteCallback(const WriteCompleteCallback& cb)
    { writeCompleteCallback_ = cb; }

    /// Internal use only.
    void setCloseCallback(const CloseCallback& cb)
    { closeCallback_ = cb; }
    // called when TcpServer accepts a new connection
    void connectEstablished();// should be called only once
    // called when TcpServer has removed me from its map
    void connectDestroyed();  // should be called only once

    void setContext(const boost::any& context)
    { context_ = context; }

    const boost::any& getContext() const
    { return context_; }

    boost::any* getMutableContext()
    { return &context_; }

    // 文件相关的context
    void setFileContext(const filectxPii& fcontext)
    {fileContext_=fcontext;}
    filectxPii* getFileContext()
    { return &fileContext_; }

    // 时间事件相关的context，TODO：几个context可以合成一个
    void setTimerContext(const boost::any& tcontext)
    {timerContext_=tcontext;}
    boost::any* getMutableTimerContext()
    { return &timerContext_; }

    void forceClose();
    void forceCloseWithDelay(double seconds);

private:
    enum StateE { kConnecting, kConnected, kDisconnecting, kDisconnected, };
    void handleRead(Timestamp receiveTime);
    void handleWrite();//handleWrite()  暂时为空。Channel 的 CloseCallback 会调用 TcpConnection::handleClose(),依此  类推
    void handleClose();
    void handleError();
    void setState(StateE s){state_=s;};
    void sendInLoop(const std::string& message);
    void forceCloseInLoop();

    const char* stateToString() const;
    
    EventLoop*loop_;
    const std::string  name_;
    StateE state_; // FIXME: use atomic variable
  // we don't expose those classes to client.
    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel>channel_;
    const InetAddress localAddr_;
    const InetAddress peerAddr_;
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    CloseCallback closeCallback_;
    Buffer inputBuffer_;
    Buffer outputBuffer_;

    boost::any context_;
    // 与文件发送有关的context
    filectxPii fileContext_;
    boost::any timerContext_;
};

}


#endif