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


class Poller:noncopyable{
public:
    Poller(EventLoop* loop);
    virtual ~Poller();
    /// Changes the interested I/O events.
    /// Must be called in the loop thread.
    virtual void updateChannel(Channel *channel)=0;
    /// Remove the channel, when it destructs.
    /// Must be called in the loop thread.
    virtual void removeChannel(Channel* channel)=0;

    /// Polls the I/O events.
    /// Must be called in the loop thread.
    virtual Timestamp poll(int timeoutMs,ChannelList* activeChannels)=0;
    void assertInLoopThread()const{ownerLoop_->assertInLoopThread();};

    virtual bool hasChannel(Channel* channel) const;

  static Poller* newDefaultPoller(EventLoop* loop);


protected:
    typedef std::map<int, Channel*> ChannelMap;
    ChannelMap channels_;
private:
    EventLoop* ownerLoop_;
};
}

#endif  // MUDUO_NET_POLLER_H