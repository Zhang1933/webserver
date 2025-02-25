
#include "muduo/net/Buffer.h"
#include "muduo/net/http/HttpContext.h"
#include <cstddef>
#include <string>

using namespace muduo;

const std::string kContentLen="Content-Length";

bool HttpContext::processRequestLine(const char* begin, const char* end)
{
  bool succeed = false;
  const char* start = begin;
  const char* space = std::find(start, end, ' ');
  if (space != end && request_.setMethod(start, space))
  {
    start = space+1;
    space = std::find(start, end, ' ');
    if (space != end)
    {
      const char* question = std::find(start, space, '?');
      if (question != space)
      {
        request_.setPath(start, question);
        request_.setQuery(question, space);
      }
      else
      {
        request_.setPath(start, space);
      }
      start = space+1;
      succeed = end-start == 8 && std::equal(start, end-1, "HTTP/1.");
      if (succeed)
      {
        if (*(end-1) == '1')
        {
          request_.setVersion(HttpRequest::kHttp11);
        }
        else if (*(end-1) == '0')
        {
          request_.setVersion(HttpRequest::kHttp10);
        }
        else
        {
          succeed = false;
        }
      }
    }
  }
  return succeed;
}

// return false if any error
bool HttpContext::parseRequest(Buffer* buf, Timestamp receiveTime)
{
  bool ok = true;
  bool hasMore = true;
  while (hasMore)
  {
    if (state_ == kExpectRequestLine)
    {
      const char* crlf = buf->findCRLF();
      if (crlf)
      {
        ok = processRequestLine(buf->peek(), crlf);
        if (ok)
        {
          request_.setReceiveTime(receiveTime);
          buf->retrieveUntil(crlf + 2);
          state_ = kExpectHeaders;
        }
        else
        {
          hasMore = false;
        }
      }
      else
      {
        hasMore = false;
      }
    }
    else if (state_ == kExpectHeaders)
    {
      const char* crlf = buf->findCRLF();
      if (crlf)
      {
        const char* colon = std::find(buf->peek(), crlf, ':');
        if (colon != crlf)
        {
          request_.addHeader(buf->peek(), colon, crlf);
        }
        else
        {
          // empty line, end of header
          if(request_.method()==HttpRequest::kPost)
          {
            state_=kExpectBody;
          }
          else{
            state_ = kGotAll;
            hasMore = false;
          }

        }
        buf->retrieveUntil(crlf + 2);
      }
      else
      {
        hasMore = false;
      }
    }
    else if (state_ == kExpectBody)
    {
        auto it=request_.headers().find(kContentLen);
        if(it!=request_.headers().end())
        {
            size_t len=std::stoul(it->second);
            if(buf->readableBytes()>=len)
            {
                request_.setBody(buf->retrieveAsString(len));
                // 剩下的不管了
                buf->retrieveAll();
                state_=kGotAll;
            }else {
              // 等待数据继续读
              state_=kExpectBody;
            }
        }
        else
        { // 没有ContentLen字段
          state_=kGotAll;
        }
        // 等待接受更多数据
        hasMore=false;
    }
  }
  return ok;
}
