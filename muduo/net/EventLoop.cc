// excerpts from http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "muduo/net/EventLoop.h"

#include "muduo/base/CurrentThread.h"
#include "muduo/base/Logging.h"
#include "muduo/net/Channel.h"
#include "muduo/net/Poller.h"

#include <assert.h>
#include <cstddef>
#include <poll.h>

using namespace muduo;


__thread EventLoop* t_loopInThisThread=0;
const int kPollTimesMs=10000;

EventLoop::EventLoop()
  :looping_(false),
  threadId_(CurrentThread::tid()),
  poller_(new Poller(this))
{
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
  quit_=false;
  while(!quit_)
  {
    activeChannels_.clear();
    poller_->poll(kPollTimesMs,&activeChannels_);
    for(ChannelList::iterator it=activeChannels_.begin();it!=activeChannels_.end();++it){
      (*it)->handleEvent();
    }
  }
  LOG_TRACE << "EventLoop " << this << " stop looping";
  looping_ = false;
}

void EventLoop::abortNotInloopThread()
{
    LOG_FATAL << "EventLoop::abortNotInLoopThread - EventLoop " << this
            << " was created in threadId_ = " << threadId_
            << ", current thread id = " <<  CurrentThread::tid();
}

void EventLoop::updateChannel(Channel *channel)
{
  assert(channel->ownerLoop()==this);
  assertInLoopThread();// 确保调用的和创建的线程相同
  poller_->updateChannel(channel);
}

void EventLoop::quit()
{
  quit_=true;
  //wakeup()
}