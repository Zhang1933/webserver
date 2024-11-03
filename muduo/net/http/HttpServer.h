#ifndef MUDUO_NET_HTTP_HTTPSERVER_H
#define MUDUO_NET_HTTP_HTTPSERVER_H

#include "muduo/net/TcpServer.h"
#include "muduo/net/http/HttpContext.h"
#include <memory>
#include <atomic>
#include <list>
#include <boost/circular_buffer.hpp>

namespace muduo
{

class HttpRequest;
class HttpResponse;

/// A simple embeddable HTTP server designed for report status of a program.
/// It is not a fully HTTP 1.1 compliant server, but provides minimum features
/// that can communicate with HttpClient and Web browser.
/// It is synchronous, just like Java Servlet.
class HttpServer : noncopyable
{
 public:
  typedef std::shared_ptr<FILE> FilePtr;
  typedef std::pair<FilePtr, bool> filectxPii;

  typedef std::function<void (const HttpRequest&,
                              HttpResponse*)> HttpCallback;

  HttpServer(EventLoop* loop,
             const InetAddress& listenAddr,
             const string& name,
             int idleSeconds,int maxConnections
             );

  EventLoop* getLoop() const { return server_.getLoop(); }

  /// Not thread safe, callback be registered before calling start().
  void setHttpCallback(const HttpCallback& cb)
  {
    httpCallback_ = cb;
  }

  void setThreadNum(int numThreads)
  {
    server_.setThreadNum(numThreads);
  }

  void start();

 private:
  void onConnection(const TcpConnectionPtr& conn);
  void onMessage(const TcpConnectionPtr& conn,
                 Buffer* buf,
                 Timestamp receiveTime);
  void onRequest(const TcpConnectionPtr&, HttpContext*);
  void onWriteComplete(const TcpConnectionPtr& conn);
  void onTimer(EventLoop* ioloop);
  
  typedef std::weak_ptr<TcpConnection> WeakTcpConnectionPtr;
  typedef std::list<WeakTcpConnectionPtr> WeakConnectionList;
  struct Node : public muduo::copyable
  {
    Timestamp lastReceiveTime;
    WeakConnectionList::iterator position;
  };
  void  dumpConnectionList(WeakConnectionList* connectionList)const;

  TcpServer server_;
  int idleSeconds_;

  HttpCallback httpCallback_;
  const int kMaxConnections_;
  std::atomic<int> numConnected_;
};

}  // namespace muduo

#endif  // MUDUO_NET_HTTP_HTTPSERVER_H
