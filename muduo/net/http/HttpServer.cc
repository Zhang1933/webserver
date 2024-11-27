#include "muduo/net/http/HttpServer.h"

#include "muduo/base/Logging.h"
#include "muduo/base/Types.h"
#include "muduo/net/Callback.h"
#include "muduo/net/EventLoop.h"
#include "muduo/net/http/HttpContext.h"
#include "muduo/net/http/HttpRequest.h"
#include "muduo/net/http/HttpResponse.h"
#include <cassert>
#include <cstddef>
#include <functional>
#include <sys/types.h>
#include <utility>

using namespace muduo;


namespace muduo
{
namespace detail
{
void defaultHttpCallback(const HttpRequest&, HttpResponse* resp)
{
  resp->setStatusCode(HttpResponse::k404NotFound);
  resp->setStatusMessage("Not Found");
  resp->setCloseConnection(true);
}
}
}

HttpServer::HttpServer(EventLoop* loop,
                       const InetAddress& listenAddr,
                       const string& name,int idleSeconds,int maxConnections)
  : server_(loop, listenAddr),
    idleSeconds_(idleSeconds),
    httpCallback_(detail::defaultHttpCallback),
    kMaxConnections_(maxConnections),
    numConnected_(0)
{
  server_.setConnectionCallback(
      std::bind(&HttpServer::onConnection, this, std::placeholders::_1));
  server_.setMessageCallback(
      std::bind(&HttpServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  server_.setWriteCompleteCallback(
    std::bind(&HttpServer::onWriteComplete, this, _1)
  );
  // 设置ontimer,清除不活跃链接。TODD:发送消息时，也更新时间，不算成idleconnection
  server_.setThreadInitCallback(
    [this](EventLoop* ioloop) {
        ioloop->setContext(WeakConnectionList());// 初始化
        ioloop->runEvery(1.0, std::bind(&HttpServer::onTimer,this,ioloop));
    });
}

void HttpServer::start()
{
  LOG_WARN << "HttpServer[" << server_.name()
    << "] starts listening on " << server_.ipPort();
  server_.start();
}

void HttpServer::onConnection(const TcpConnectionPtr& conn)
{
  LOG_INFO << "HttpServer - " << conn->peerAddress().toHostPort() << " -> "
           << conn->localAddress().toHostPort() << " is "
           << (conn->connected() ? "UP" : "DOWN");
  WeakConnectionList* connectionList= boost::any_cast<WeakConnectionList>(conn->getLoop()->getMutableContext());
  if (conn->connected())
  {
    ++numConnected_;
    if (numConnected_ > kMaxConnections_)
    {
      LOG_DEBUG<<"excceed MaxConnections,shutdown conn,numConnected:"<<numConnected_;
      conn->shutdown();
      conn->forceCloseWithDelay(3.0);  // > round trip of the whole Internet.
    }
    Node node;
    node.lastReceiveTime = Timestamp::now(); 
    connectionList->push_back(conn);
    node.position = --connectionList->end();
    conn->setTimerContext(node);

    conn->setContext(HttpContext());
  }
  else
  {
    assert(!conn->getMutableTimerContext()->empty());
    Node* node = boost::any_cast<Node>(conn->getMutableTimerContext());
    connectionList->erase(node->position);
    conn->getMutableTimerContext()->clear();
    --numConnected_;
  }
  dumpConnectionList(connectionList);
}

void HttpServer::onMessage(const TcpConnectionPtr& conn,
                           Buffer* buf,
                           Timestamp receiveTime)
{
  HttpContext* context = boost::any_cast<HttpContext>(conn->getMutableContext());

  assert(!conn->getMutableTimerContext()->empty());
  conn->getLoop()->assertInLoopThread();

  Node* node = boost::any_cast<Node>(conn->getMutableTimerContext());
  node->lastReceiveTime = receiveTime;
  WeakConnectionList* connectionList= boost::any_cast<WeakConnectionList>(conn->getLoop()->getMutableContext());
  connectionList->splice(connectionList->end(), *connectionList, node->position);
  assert(node->position == --connectionList->end());

  dumpConnectionList(connectionList);

  if (!context->parseRequest(buf, receiveTime))
  {
    conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
    conn->shutdown();
  }

  if (context->gotAll())
  {
    onRequest(conn, context);
    context->reset();
  }
}

void HttpServer::onRequest(const TcpConnectionPtr& conn, HttpContext* context)
{
  HttpRequest& req=context->request();
  const string& connection = req.getHeader("Connection");
  bool close = connection == "close" ||
    (req.getVersion() == HttpRequest::kHttp10 && connection != "keep-alive");
  LOG_DEBUG<<"keep-alvie?close:"<<close;
  HttpResponse response(close);
  httpCallback_(req, &response);
  Buffer buf;
  if(response.appendToBuffer(&buf))
  {
    FILE* fp = ::fopen(response.GetretFilePath().c_str(), "rb");
    assert(fp);
    TcpConnection::FilePtr ctx(fp, ::fclose);
    conn->setFileContext(TcpConnection::filectxPii(ctx,close));
  }
  else 
  {
    conn->setFileContext(TcpConnection::filectxPii(TcpConnection::FilePtr(),close));
  }
  conn->send(&buf);
}

void HttpServer::onWriteComplete(const TcpConnectionPtr& conn)
{
  //NOTE: 文件发送，只取决于连接数，不取决于文件大小，drawback：不能支持pipeline。
  const int kBufSize = 64*1024;
  char buf[kBufSize];
  TcpConnection::filectxPii fpii = conn->getFileContext();
  if(fpii.first)
  {
    size_t nread = ::fread(buf, 1, sizeof buf, fpii.first.get());
    if (nread > 0)
    {
      conn->send(buf, nread);
    }
    else 
    {
        fpii.first.reset(); // FIXME:错误处理
        if(fpii.second)
        {
          conn->shutdown();
        }
        LOG_INFO << "FileSend - done";
    }
  }
  else if(fpii.second)
  {
    conn->shutdown();
  }
}

void HttpServer::dumpConnectionList(WeakConnectionList* connectionList_) const
{
  if(muduo::Logger::logLevel() > muduo::Logger::DEBUG)return;
  LOG_INFO << "size = " << connectionList_->size();

  for (WeakConnectionList::const_iterator it = connectionList_->begin();
      it != connectionList_->end(); ++it)
  {
    TcpConnectionPtr conn = it->lock();
    if (conn)
    {
      printf("conn %p ,", (conn.get()));
      Node* n = boost::any_cast<Node>(conn->getMutableTimerContext());
      printf("    time %s\n", n->lastReceiveTime.toString().c_str());
    }
    else
    {
      printf("expired\n");
    }
  }
}

void HttpServer::onTimer(EventLoop* ioloop)
{
  ioloop->assertInLoopThread();
  Timestamp now = Timestamp::now();
  WeakConnectionList* connectionList= boost::any_cast<WeakConnectionList>(ioloop->getMutableContext());
  dumpConnectionList(connectionList);
    for (WeakConnectionList::iterator it = connectionList->begin();
      it != connectionList->end();)
  {
    TcpConnectionPtr conn = it->lock();
    if (conn)
    {
      Node* n = boost::any_cast<Node>(conn->getMutableTimerContext());
      double age = timeDifference(now, n->lastReceiveTime);
      if (age > idleSeconds_)
      {
        if (conn->connected())
        {
          conn->shutdown();
          LOG_INFO << "shutting down " << conn->name();
          conn->forceCloseWithDelay(3.5);  // > round trip of the whole Internet.
        }
      }
      else if (age < 0)
      {
        LOG_WARN << "Time jump";
        n->lastReceiveTime = now;
      }
      else
      {
        break;
      }
      ++it;
    }
    else
    {
      LOG_WARN << "Expired";
      it = connectionList->erase(it);
    }
  }
}