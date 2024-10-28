#ifndef MUDUO_NET_CALLBACKS_H
#define MUDUO_NET_CALLBACKS_H

#include <functional>
#include <memory>
#include <sys/types.h>
namespace muduo {

class TcpConnection;
typedef  std::shared_ptr<TcpConnection> TcpConnectionPtr;
// All client visible callbacks go here.
typedef std::function<void()> TimerCallback;
typedef std::function<void (const TcpConnectionPtr&)> ConnectionCallback;
typedef std::function<void (const TcpConnectionPtr&,
                            const char*,
                            ssize_t)> MessageCallback;
typedef std::function<void(const TcpConnectionPtr&)>CloseCallback;

}

#endif