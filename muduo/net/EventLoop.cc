// excerpts from http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "muduo/net/EventLoop.h"

#include "muduo/base/CurrentThread.h"
#include "muduo/base/Logging.h"
#include "muduo/base/Mutex.h"
#include "muduo/base/Timestamp.h"
#include "muduo/net/Channel.h"
#include "muduo/net/Poller.h"
#include "muduo/net/TimerId.h"
#include "muduo/net/TimerQueue.h"

#include <assert.h>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <poll.h>
#include <type_traits>
#include <unistd.h>
#include <vector>

#include <sys/eventfd.h>

using namespace muduo;


__thread EventLoop* t_loopInThisThread=0;
const int kPollTimesMs=10000;

static int createEventfd()
{
  int evtfd=::eventfd(0,EFD_NONBLOCK | EFD_CLOEXEC);
  if(evtfd<0)
  {
    LOG_SYSERR << "Failed in eventfd";
     abort();
  }
  return  evtfd;
}

EventLoop::EventLoop()
  :looping_(false),
  quit_(false),
  callingPendingFunctors_(false),
  threadId_(CurrentThread::tid()),
  poller_(new Poller(this)),
  timerQueue_(new TimerQueue(this)),
  wakeupFd_(createEventfd()),
  wakeupChannel_(new Channel(this,wakeupFd_))
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
  wakeupChannel_->setReadCallback(std::bind( &EventLoop::handleRead,this));
  // we are always reading the wakeupfd
  wakeupChannel_->enableReading();
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
    pollReturnTime_=poller_->poll(kPollTimesMs,&activeChannels_);
    for(ChannelList::iterator it=activeChannels_.begin();it!=activeChannels_.end();++it)
    {
      (*it)->handleEvent(pollReturnTime_);
    }
    doPendingFunctors();
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
  if(!isInLoopThread())
  {
    wakeup(); //在 IO 线程调用 quit() 就不必 wakeup()，说明没有阻塞在poll
  }
}

TimerId  EventLoop::runAt(const Timestamp&time,const TimerCallback &cb){
    return timerQueue_->addTimer(cb, time, 0);
}


TimerId EventLoop::runAfter(double delay,const TimerCallback&cb)
{
  Timestamp time(addTime(Timestamp::now(),delay));
  return runAt(time, cb);
}

TimerId EventLoop::runEvery(double interval, const TimerCallback &cb)
{
  Timestamp time(addTime(Timestamp::now(), interval));
  return timerQueue_->addTimer(cb, time, interval);
}

void EventLoop::runInLoop(const Functor &cb)
{
  if(isInLoopThread())
  {
    cb();
  }
  else
  {
    queueInLoop(cb);
  }
}

void EventLoop::queueInLoop(const Functor &cb)
{
  {
    MutexLockGuard lock(mutex_);
    pendingFunctors_.push_back(cb);
  }
  if(!isInLoopThread()||callingPendingFunctors_)
  {
    wakeup();
  }// 只有在 IO 线程的事件回调中调用 queueInLoop() 才无须  wakeup()
}

void EventLoop::wakeup()
{
  uint64_t one=1;
  ssize_t n=::write(wakeupFd_, &one, sizeof one);
  if(n!=sizeof one)
  {
    LOG_ERROR << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
  }
}

void EventLoop::doPendingFunctors()
{
  std::vector<Functor> functors;
  callingPendingFunctors_=true;
  {
    MutexLockGuard lock(mutex_);
    functors.swap(pendingFunctors_);
  }

  for(auto i:functors)
  {
    i();
  }
  callingPendingFunctors_=false;
}

void EventLoop::handleRead()
{
  uint64_t one=1;
  ssize_t n=::read(wakeupFd_, &one, sizeof one);
  if(n!=sizeof one)
  {
     LOG_ERROR << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
  }
}

void EventLoop::removeChannel(Channel* channel)
{
  assert(channel->ownerLoop() == this);
  assertInLoopThread();
  poller_->removeChannel(channel);
}
