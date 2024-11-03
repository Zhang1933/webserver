#include "muduo/net/http/HttpServer.h"

#include "muduo/base/Logging.h"
#include "muduo/net/http/HttpContext.h"
#include "muduo/net/http/HttpRequest.h"
#include "muduo/net/http/HttpResponse.h"
#include <cassert>
#include <functional>
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
                       const string& name)
  : server_(loop, listenAddr),
    httpCallback_(detail::defaultHttpCallback)
{
  server_.setConnectionCallback(
      std::bind(&HttpServer::onConnection, this, std::placeholders::_1));
  server_.setMessageCallback(
      std::bind(&HttpServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  server_.setWriteCompleteCallback(
    std::bind(&HttpServer::onWriteComplete, this, _1)
  );
}

void HttpServer::start()
{
  LOG_WARN << "HttpServer[" << server_.name()
    << "] starts listening on " << server_.ipPort();
  server_.start();
}

void HttpServer::onConnection(const TcpConnectionPtr& conn)
{
  if (conn->connected())
  {
    conn->setContext(HttpContext());
  }
}
// 一个连接要发多个文件
void HttpServer::onMessage(const TcpConnectionPtr& conn,
                           Buffer* buf,
                           Timestamp receiveTime)
{
  HttpContext* context = boost::any_cast<HttpContext>(conn->getMutableContext());

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
  bool close = connection == "close";
  HttpResponse response(close);
  httpCallback_(req, &response);
  Buffer buf;
  if(response.appendToBuffer(&buf))
  {
    FILE* fp = ::fopen(response.GetretFilePath().c_str(), "rb");
    assert(fp);
    FilePtr ctx(fp, ::fclose);
    conn->setFileContext(filectxPii(ctx,close));

  }
  else 
  {
    conn->setFileContext(filectxPii(FilePtr(),close));
  }
  conn->send(&buf);
}

void HttpServer::onWriteComplete(const TcpConnectionPtr& conn)
{
  //NOTE: 文件发送，只取决于连接数，不取决于文件大小，drawback：不能支持pipeline。
  const int kBufSize = 64*1024;
  char buf[kBufSize];
  filectxPii& fpii = boost::any_cast<filectxPii&>(conn->getFileContext());
  if(fpii.first)
  {
    size_t nread = ::fread(buf, 1, sizeof buf, fpii.first.get());
    if (nread > 0)
    {
      conn->send(buf, nread);
    }
    else 
    {
        fpii.first.reset();
        if(fpii.second)
        {
          conn->shutdown();
        }
        LOG_INFO << "FileSend - done";
    }
  }
  else
  {
      if(fpii.second)
      {
        conn->shutdown();
      }
  }
}