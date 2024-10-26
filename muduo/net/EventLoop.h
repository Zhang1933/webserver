// excerpts from http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_NET_EVENTLOOP_H
#define MUDUO_NET_EVENTLOOP_H

#include <boost/any.hpp>
#include <sched.h>

#include "muduo/base/Mutex.h"
#include "muduo/base/CurrentThread.h"

namespace muduo
{
namespace  net
{

class EventLoop : noncopyable
{
public:
  EventLoop();
  ~EventLoop();

  void loop();

  void assertInLoopThread(){
    if(!isInLoopThread())
    {
      abortNotInloopThread();
    }
  }

  bool isInLoopThread()const{return threadId_==CurrentThread::tid();};

private:
  void abortNotInloopThread();

  bool looping_; /*atomic*/
  const pid_t threadId_;

};

}  // namespace net
} // namespace muduo

#endif  // MUDUO_NET_EVENTLOOP_H
