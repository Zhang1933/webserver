#ifndef MUDUO_NET_TCPCONNECTION_H
#define MUDUO_NET_TCPCONNECTION_H

#include "muduo/base/noncopyable.h"
#include "muduo/net/Callback.h"
#include "muduo/net/InetAddress.h"
#include "muduo/net/Buffer.h"
#include <memory>
#include <string>
#include <boost/any.hpp>

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

    void setFileContext(const boost::any& fcontext)
    {fileContext_=fcontext;}
    boost::any& getFileContext()
    { return fileContext_; }
    boost::any* getMutableFileContext()
    { return &fileContext_; }

private:
    enum StateE { kConnecting, kConnected, kDisconnecting, kDisconnected, };
    void handleRead(Timestamp receiveTime);
    void handleWrite();//handleWrite()  暂时为空。Channel 的 CloseCallback 会调用 TcpConnection::handleClose(),依此  类推
    void handleClose();
    void handleError();
    void setState(StateE s){state_=s;};
    void sendInLoop(const std::string& message);

    const char* stateToString() const;
    
    EventLoop*loop_;
    std::string  name_;
    StateE state_; // FIXME: use atomic variable
  // we don't expose those classes to client.
    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel>channel_;
    InetAddress localAddr_;
    InetAddress peerAddr_;
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    CloseCallback closeCallback_;
    Buffer inputBuffer_;
    Buffer outputBuffer_;

    boost::any context_;
    // 与文件发送有关的context
    boost::any fileContext_;
};

}


#endif