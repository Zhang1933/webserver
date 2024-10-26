// excerpts from http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "muduo/net/EventLoop.h"

#include "muduo/base/CurrentThread.h"
#include "muduo/base/Logging.h"

#include <assert.h>
#include <cstddef>
#include <poll.h>
#include <sys/poll.h>

using namespace muduo;
using namespace muduo::net;

namespace {
  __thread EventLoop* t_loopInThisThread=0;
}

EventLoop::EventLoop():looping_(false),threadId_(CurrentThread::tid()){
  LOG_TRACE << "EventLoop created " << this << " in thread " << threadId_;
  if (t_loopInThisThread)
  {
    LOG_FATAL << "Another EventLoop " << t_loopInThisThread
              << " exists in this thread " << threadId_;
  }
  else
  {
    t_loopInThisThread=this;
  }
}

EventLoop::~EventLoop()
{
  assert(!looping_);
  t_loopInThisThread=NULL;
}

void EventLoop::loop()
{
  assert(!looping_);
  assertInLoopThread();
  looping_=true;
  ::poll(NULL, 0, 5*100000);
  LOG_TRACE << "EventLoop " << this << " stop looping";
  looping_=false;
}

void EventLoop::abortNotInloopThread()
{
    LOG_FATAL << "EventLoop::abortNotInLoopThread - EventLoop " << this
            << " was created in threadId_ = " << threadId_
            << ", current thread id = " <<  CurrentThread::tid();
}