// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.

#ifndef MUDUO_NET_HTTP_HTTPRESPONSE_H
#define MUDUO_NET_HTTP_HTTPRESPONSE_H

#include "muduo/base/copyable.h"
#include "muduo/base/Types.h"
#include "muduo/net/Callback.h"

#include <map>
#include <string>
#include <string_view>

namespace muduo
{


class Buffer;
class HttpResponse : public muduo::copyable
{
 public:

 static const  std::string RootPath;
  enum HttpStatusCode
  {
    kUnknown,
    k200Ok = 200,
    k301MovedPermanently = 301,
    k400BadRequest = 400,
    k404NotFound = 404,
  };

  explicit HttpResponse(bool close)
    : statusCode_(k200Ok),
    statusMessage_("OK"),
    closeConnection_(close)
  {
  }

  void setCookie(const string& cook,int Max_Age=-1){
    cookie_=cook+";";
  }

  void setStatusCode(HttpStatusCode code)
  { statusCode_ = code; }

  void setStatusMessage(const string& message)
  { statusMessage_ = message; }

  void setCloseConnection(bool on)
  { closeConnection_ = on; }

  bool closeConnection() const
  { return closeConnection_; }

  void setContentType(const string& contentType)
  { addHeader("Content-Type", contentType); }

  // FIXME: replace string with StringPiece
  void addHeader(const string& key, const string& value)
  { headers_[key] = value; }

  void setBody(const string& body)
  { body_ = body; }

  void setRetfilePath(const string&path)
  {retFilePath_=RootPath+path;}

  // append的时候判断有没有文件需要返回
  bool appendToBuffer(Buffer* output);

  string GetretFilePath()const
  {return retFilePath_;}

 private:
  std::map<string, string> headers_;
  HttpStatusCode statusCode_;
  // FIXME: add http version
  string statusMessage_;
  string cookie_;
  bool closeConnection_;
  string body_;
  string retFilePath_;
};

}  // namespace muduo

#endif  // MUDUO_NET_HTTP_HTTPRESPONSE_H
