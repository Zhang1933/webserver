#include "muduo/net/EventLoopThread.h"
#include "muduo/base/Condition.h"
#include "muduo/base/Mutex.h"
#include "muduo/base/Thread.h"
#include "muduo/net/EventLoop.h"
#include <cassert>
#include <functional>

using namespace muduo;

EventLoopThread::EventLoopThread(const ThreadInitCallback& cb,
                                 const string& name)
    :loop_(nullptr),
    exiting_(false),
    thread_(std::bind(&EventLoopThread::threadFunc,this)),
    mutex_(),
    cond_(mutex_),
    callback_(cb)
{
}

EventLoopThread::~EventLoopThread()
{
    exiting_=true;
    if (loop_ != NULL) // not 100% race-free, eg. threadFunc could be running callback_.
    {
        // still a tiny chance to call destructed object, if threadFunc exits just now.
        // but when EventLoopThread destructs, usually programming is exiting anyway.
        loop_->quit();
        thread_.join();
    }
}

EventLoop* EventLoopThread::startLoop()
{
    assert(!thread_.started());
    thread_.start();
    {
        MutexLockGuard lock(mutex_);
        while(loop_==NULL)
        {
            cond_.wait();
        }
    }
    return loop_;
}

// loop 线程
void EventLoopThread::threadFunc()
{
    EventLoop loop;
    if (callback_)
    {
        callback_(&loop);
    }

    {
        MutexLockGuard lock(mutex_);
        loop_=&loop;
        cond_.notify();
    }
    loop.loop();
    //assert(exiting_);
}