// excerpts from http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_NET_EVENTLOOP_H
#define MUDUO_NET_EVENTLOOP_H

#include <boost/any.hpp>
#include <functional>
#include <memory>
#include <sched.h>
#include <vector>

#include "muduo/base/Timestamp.h"
#include "muduo/base/Mutex.h"
#include "muduo/base/CurrentThread.h"

#include "muduo/net/Timer.h"
#include "muduo/net/TimerId.h"
#include "muduo/net/Callback.h"

namespace muduo
{

class Channel;
class Poller;
class TimerQueue;

typedef std::vector<Channel*>ChannelList;
class EventLoop : noncopyable
{
public:
  typedef std::function<void()> Functor;
  EventLoop();
  
  // force out-line dtor, for scoped_ptr members.
  ~EventLoop();

  ///
  /// Loops forever.
  ///
  /// Must be called in the same thread as creation of the object.
  ///
  void loop();
  void quit();

  /// Runs callback immediately in the loop thread.
  /// It wakes up the loop, and run the cb.
  /// If in the same loop thread, cb is run within the function.
  /// Safe to call from other threads.
  void runInLoop(const Functor& cb);
    /// Queues callback in the loop thread.
  /// Runs after finish pooling.
  /// Safe to call from other threads.
  void queueInLoop(const Functor& cb);

  // timers
  ///
  /// Runs callback at 'time'.
  ///
  TimerId runAt(const Timestamp&time,const TimerCallback &cb);
  ///
  /// Runs callback every @c interval seconds.
  ///
  TimerId runEvery(double interval,const TimerCallback&cb);
  ///
  /// Runs callback after @c delay seconds.
  ///
  TimerId runAfter(double delay,const TimerCallback&cb);

  // void cancel(TimerId timerId);
  
  // internal use only
  void wakeup();
  void updateChannel(Channel* channel);
  void removeChannel(Channel* channel);

  void assertInLoopThread(){
    if(!isInLoopThread())
    {
      abortNotInloopThread();
    }
  }

  bool isInLoopThread()const{return threadId_==CurrentThread::tid();};

private:

  void abortNotInloopThread();
  void handleRead(); // waked up
  void doPendingFunctors();
  
  ChannelList activeChannels_;
  bool looping_; /*atomic*/
  bool quit_;
  bool callingPendingFunctors_;
  const pid_t threadId_;
  Timestamp pollReturnTime_;

  std::unique_ptr<Poller> poller_;
  std::unique_ptr<TimerQueue>timerQueue_;
  int wakeupFd_;
    // unlike in TimerQueue, which is an internal class,
  // we don't expose Channel to client.
  std::unique_ptr<Channel>wakeupChannel_;
  MutexLock mutex_;
  std::vector<Functor>pendingFunctors_; // @GuardedBy mutex_
};
} // namespace muduo

#endif  // MUDUO_NET_EVENTLOOP_H
