#ifndef MUDUO_NET_CALLBACKS_H
#define MUDUO_NET_CALLBACKS_H

#include <functional>
#include <memory>
#include <sys/types.h>
#include "muduo/base/Timestamp.h"

namespace muduo {

class TcpConnection;
class Buffer;
typedef  std::shared_ptr<TcpConnection> TcpConnectionPtr;
// All client visible callbacks go here.
typedef std::function<void()> TimerCallback;
typedef std::function<void (const TcpConnectionPtr&)> ConnectionCallback;
typedef std::function<void (const TcpConnectionPtr&,
                              Buffer* buf,
                              Timestamp)> MessageCallback;
typedef std::function<void (const TcpConnectionPtr&)> WriteCompleteCallback;
typedef std::function<void(const TcpConnectionPtr&)>CloseCallback;

}

#endif