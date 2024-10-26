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

#include "muduo/base/Mutex.h"
#include "muduo/base/CurrentThread.h"
namespace muduo
{

class Channel;
class Poller;

typedef std::vector<Channel*>ChannelList;

class EventLoop : noncopyable
{
public:
  EventLoop();
  ~EventLoop();

  void loop();
  void quit();

  void assertInLoopThread(){
    if(!isInLoopThread())
    {
      abortNotInloopThread();
    }
  }

  // internal use only
  void updateChannel(Channel* channel);
  bool isInLoopThread()const{return threadId_==CurrentThread::tid();};

private:
  void abortNotInloopThread();

  
  bool looping_; /*atomic*/
  bool quit_;
  const pid_t threadId_;

  ChannelList activeChannels_;
  std::unique_ptr<Poller> poller_;
};
} // namespace muduo

#endif  // MUDUO_NET_EVENTLOOP_H
