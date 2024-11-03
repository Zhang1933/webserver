#ifndef MUDUO_NET_EVENTLOOPTHREAD_H
#define MUDUO_NET_EVENTLOOPTHREAD_H

#include "muduo/base/Condition.h"
#include "muduo/base/Mutex.h"
#include "muduo/base/Thread.h"
#include "muduo/base/noncopyable.h"

namespace muduo {

class EventLoop;

class EventLoopThread:noncopyable{
public:
typedef std::function<void(EventLoop*)> ThreadInitCallback;
    EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback(),
                  const string& name = string());
    ~EventLoopThread();


    EventLoop* startLoop();

private:
    void threadFunc();
    EventLoop * loop_; // 栈上对象
    bool exiting_;
    Thread thread_;
    MutexLock mutex_;
    Condition cond_;
    ThreadInitCallback callback_;
};
}

#endif