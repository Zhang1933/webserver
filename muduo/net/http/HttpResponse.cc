#include "muduo/net/http/HttpResponse.h"
#include "muduo/net/TcpConnection.h"
#include "muduo/net/Buffer.h"
#include <string>
#include <sys/stat.h>
#include "muduo/base/FileUtil.h"
#include <stdio.h>

using namespace muduo;

bool HttpResponse::appendToBuffer(Buffer* output)
{
  char buf[32];
  bool hasfile=false;
  snprintf(buf, sizeof buf, "HTTP/1.1 %d ", statusCode_);
  output->append(buf);
  output->append(statusMessage_);
  output->append("\r\n");
  if (closeConnection_)
  {
    output->append("Connection: close\r\n");
  }
  else
  {
    output->append("Connection: keep-alive\r\n");
  }
  
  for (const auto& header : headers_)
  {
    output->append(header.first);
    output->append(": ");
    output->append(header.second);
    output->append("\r\n");
  }
  
  if(!this->cookie_.empty()){
      output->append("Set-Cookie: "+this->cookie_);
      output->append("\r\n");
  }
  
  if(retFilePath_.empty())
  {
    snprintf(buf, sizeof buf, "Content-Length: %zd\r\n", body_.size());
    output->append(buf);
    output->append("\r\n");
    output->append(body_);
  }
  else
  {
    struct stat stat_buf;
    int rc = stat(retFilePath_.c_str(), &stat_buf);
    if(!rc)
    {
        snprintf(buf, sizeof buf, "Content-Length: %zd\r\n",stat_buf.st_size);
        output->append(buf);
        output->append("\r\n");
        hasfile=true;
    }
    else
    {
        // 找不到文件
        setCloseConnection(true);
        output->append("Content-Length: 29\r\n");
        output->append("\r\n");
        output->append("404,file not found");
    }
  }
  return hasfile;
}

std::string getPwd(){
  std::string rootPath;
  char buffer[1024];
  char* tmp=getcwd(buffer, 1024);
  (void)tmp;
  rootPath=buffer;
  rootPath+="/root/";
  return rootPath;
}

const std::string HttpResponse::RootPath=getPwd();