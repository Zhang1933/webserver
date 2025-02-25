#ifndef MUDUO_NET_CHANNEL_H
#define MUDUO_NET_CHANNEL_H

#include "muduo/base/Mutex.h"
#include "muduo/base/noncopyable.h"
#include <functional>
#include "muduo/net/EventLoop.h"

namespace muduo {

class Channel:noncopyable
{
public:
    typedef  std::function<void()>EventCallback;
    typedef std::function<void(Timestamp)> ReadEventCallback;

    Channel(EventLoop *loop,int fd);
    ~Channel();
    void handleEvent(Timestamp receiveTime);
    void setReadCallback(const ReadEventCallback &cb)\
    { readCallback_=cb; }
      void setWriteCallback(const EventCallback& cb)
    { writeCallback_ = cb; }
    void setErrorCallback(const EventCallback& cb)
    { errorCallback_ = cb; }
    void setCloseCallback(const EventCallback& cb)
    { closeCallback_ = cb; }
   
    EventLoop*ownerLoop(){return loop_;}
    int fd()const {return fd_;}
    int events()const {return events_;}
    void set_revents(__uint32_t revt){revents_=revt;}

    // for poller;
    int index()const{return index_;}
    void set_index(int idx){index_=idx;}
    bool isNoneEvent()const{return events_==kNoneEvent;}

    void enableReading() { events_ |= kReadEvent;update();}
    void enableWriting(){events_|=kWriteEvent;update();}
    void disableWriting() { events_ &= ~kWriteEvent; update(); }
    void disableAll() { events_ = kNoneEvent; update(); }
    bool isWriting() const { return (events_ & kWriteEvent)<<1; }
    
    void remove();
    
      // for debug
  string reventsToString() const;
  string eventsToString() const;

private:
    void update();
    static string eventsToString(int fd, __uint32_t ev);

    static const __uint32_t kNoneEvent;
    static const __uint32_t kReadEvent;
    static const __uint32_t kWriteEvent;

    EventLoop* loop_;
    const int fd_;
    __uint32_t events_; //其中 events_ 是它关心的 IO 事件,由用户设 置;
    __uint32_t revents_;//revents_ 是目前活动的事件
    int index_; // used by poller

    bool eventHandling_;
    bool addedToLoop_;

    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback errorCallback_;
    EventCallback closeCallback_;
};

}


#endif  // MUDUO_NET_CHANNEL_H