// excerpts from http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_NET_EVENTLOOP_H
#define MUDUO_NET_EVENTLOOP_H

#include <boost/any.hpp>
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

  // TODO:返回值返回的原始指针，可能有问题。
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
  void updateChannel(Channel* channel);

  void assertInLoopThread(){
    if(!isInLoopThread())
    {
      abortNotInloopThread();
    }
  }

  bool isInLoopThread()const{return threadId_==CurrentThread::tid();};

  ///
  /// Runs callback at 'time'.
  ///
  

private:
  void abortNotInloopThread();

  
  bool looping_; /*atomic*/
  bool quit_;
  const pid_t threadId_;

  ChannelList activeChannels_;
  std::unique_ptr<Poller> poller_;
  std::unique_ptr<TimerQueue>timerQueue_;
};
} // namespace muduo

#endif  // MUDUO_NET_EVENTLOOP_H
