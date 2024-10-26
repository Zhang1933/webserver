#ifndef MUDUO_NET_POLLER_H
#define MUDUO_NET_POLLER_H

#include "muduo/base/Timestamp.h"
#include "muduo/base/noncopyable.h"
#include "muduo/net/EventLoop.h"
#include <map>
#include <poll.h>
#include <vector>

namespace muduo
{
class Channel;
class EventLoop;

class Poller:noncopyable{
public:
    Poller(EventLoop* loop);
    ~Poller();
    /// Changes the interested I/O events.
    /// Must be called in the loop thread.
    void updateChannel(Channel *channel);

    /// Polls the I/O events.
    /// Must be called in the loop thread.
    Timestamp poll(int timeoutMs,ChannelList* activeChannels);
    void assertInLoopThread(){ownerLoop_->assertInLoopThread();};

private:
    EventLoop* ownerLoop_;
    typedef std::map<int, Channel*> ChannelMap;
    typedef std::vector<struct pollfd> PollFdList;

    PollFdList pollfds_;
    ChannelMap channels_;

    void fillActiveChannels(int numEvents,
        ChannelList* activeChannels
    )const;
};
}

#endif  // MUDUO_NET_POLLER_H