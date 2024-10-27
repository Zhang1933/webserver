#ifndef MUDUO_NET_CALLBACKS_H
#define MUDUO_NET_CALLBACKS_H

#include <functional>
namespace muduo {

// All client visible callbacks go here.
typedef std::function<void()> TimerCallback;
}

#endif