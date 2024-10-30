// This is an internal header file, you should not include this.

#ifndef MUDUO_NET_EVENTLOOPTHREADPOOL_H
#define MUDUO_NET_EVENTLOOPTHREADPOOL_H

#include "muduo/base/noncopyable.h"
#include "muduo/base/Types.h"

#include <functional>
#include <memory>
#include <unistd.h>
#include <vector>

namespace muduo
{

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : noncopyable
{
 public:

 typedef std::function<void(EventLoop*)> ThreadInitCallback;
  EventLoopThreadPool(EventLoop* baseLoop, const string& nameArg=string());
  ~EventLoopThreadPool();
  void setThreadNum(int numThreads) { numThreads_ = numThreads; }
  void start(const ThreadInitCallback& cb = ThreadInitCallback());

    // valid after calling start()
    /// round-robin
    EventLoop* getNextLoop();

    std::vector<EventLoop*> getAllLoops();


 private:
  EventLoop* baseLoop_;
  string name_;
  bool started_;
  int numThreads_;
  size_t next_;  // always in loop thread
  std::vector<std::unique_ptr<EventLoopThread>> threads_;
  std::vector<EventLoop*> loops_;
};

}
#endif  // MUDUO_NET_EVENTLOOPTHREADPOOL_H
