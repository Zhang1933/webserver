#include "Poller.h"
#include "muduo/base/Logging.h"
#include "muduo/base/Timestamp.h"
#include "muduo/net/Channel.h"
#include "muduo/net/EventLoop.h"
#include <cassert>
#include <poll.h>
#include <unistd.h>
#include <zconf.h>

using namespace muduo;

Poller::Poller(EventLoop* loop)
    :ownerLoop_(loop)
{
}

Poller::~Poller()
{
}

void Poller::updateChannel(Channel * channel)
{
    assertInLoopThread();// 线程会监听多个fd,TODO:文件描述符管只能给一个channel管？
    LOG_TRACE << "fd = " << channel->fd() << " events = " << channel->events();
    if(channel->index()<0)
    {
        //a new one,add to pollfds_
        assert(channels_.find(channel->fd())==channels_.end());
        struct pollfd pfd;
        pfd.fd=channel->fd();
        pfd.events=static_cast<short>(channel->events());
        pfd.revents=0;
        pollfds_.push_back(pfd);
        int idx=static_cast<int>(pollfds_.size())-1;
        channel->set_index(idx);
        channels_[pfd.fd]=channel;
    }else{
        // update existing one
        assert(channels_.find(channel->fd())!=channels_.end());
        assert(channels_[channel->fd()]==channel);
        int idx=channel->index();
        assert(0 <= idx && idx < static_cast<int>(pollfds_.size()));
        struct pollfd &pfd=pollfds_[static_cast<size_t>(idx)];
        assert(pfd.fd==channel->fd()||pfd.fd==-channel->fd()-1);
        pfd.events=static_cast<short>(channel->events());
        pfd.revents=0;
        if(channel->isNoneEvent()){
            // ignore this pollfd
            pfd.fd=-channel->fd()-1;
        }
    }
}

Timestamp Poller::poll(int timeoutMs,ChannelList* activeChannels)
{
    int numEvents=::poll(&*pollfds_.begin(),pollfds_.size(),timeoutMs);
    Timestamp now(Timestamp::now());
    if(numEvents>0){
        LOG_TRACE << numEvents << " events happended";
        fillActiveChannels(numEvents,activeChannels);
    }else if(numEvents==0){
        LOG_TRACE << " nothing happended";
    }else{
         LOG_SYSERR << "Poller::poll()";
    }
    return now;
}


void Poller::fillActiveChannels(int numEvents,
                                ChannelList* activeChannels)const
{
    for(auto pfd=pollfds_.begin();pfd!=pollfds_.end()&&numEvents>0;++pfd)
    {
        if(pfd->revents>0)
        {
            --numEvents;
            const auto ch=channels_.find(pfd->fd);
            assert(ch!=channels_.end());
            Channel* channel=ch->second;
            assert(channel->fd()==pfd->fd);
            channel->set_revents(pfd->revents);
            activeChannels->push_back(channel);
        }
    }
}


void Poller::removeChannel(Channel* channel)
{
  assertInLoopThread();
  LOG_TRACE << "fd = " << channel->fd();
  assert(channels_.find(channel->fd()) != channels_.end());
  assert(channels_[channel->fd()] == channel);
  assert(channel->isNoneEvent());
  int idx = channel->index();
  assert(0 <= idx && idx < static_cast<int>(pollfds_.size()));
  const struct pollfd& pfd = pollfds_[static_cast<size_t>(idx)]; (void)pfd;
  assert(pfd.fd == -channel->fd()-1 && pfd.events == channel->events());
  size_t n = channels_.erase(channel->fd());
  assert(n == 1); (void)n;
  if (implicit_cast<size_t>(idx) == pollfds_.size()-1) {
    pollfds_.pop_back();
  } else {
    int channelAtEnd = pollfds_.back().fd;
    iter_swap(pollfds_.begin()+idx, pollfds_.end()-1);
    if (channelAtEnd < 0) {
      channelAtEnd = -channelAtEnd-1;
    }
    channels_[channelAtEnd]->set_index(idx); //注意其中从数组 pollfds_ 中删除元素是 O(1) 复杂度,办法是将待删除的元素与 最后一个元素交换,再 pollfds_.pop_back()
    pollfds_.pop_back();
  }
}
