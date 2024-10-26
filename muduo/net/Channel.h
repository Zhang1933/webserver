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

    Channel(EventLoop *loop,int fd);
    void handleEvent();
    void setReadCallback(const EventCallback &cb)\
    { readCallback_=cb; }
      void setWriteCallback(const EventCallback& cb)
    { writeCallback_ = cb; }
    void setErrorCallback(const EventCallback& cb)
    { errorCallback_ = cb; }
   
    EventLoop*ownerLoop(){return loop_;}
    int fd()const {return fd_;}
    int events()const {return events_;}
    void set_revents(int revt){revents_=revt;}

    // for poller;
    int index()const{return index_;}
    void set_index(int idx){index_=idx;}
    bool isNoneEvent()const{return events_==kNoneEvent;}

    void enableReading() { events_ |= kReadEvent;update();}
    // void enableWriting(){events_|=kWriteEvent;update();}
    // void disableWriting(){events_&=!kWriteEvent;update();}
    // void disableAll() { events_ = kNoneEvent; update(); }

private:
    void update();

    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop* loop_;
    const int fd_;
    int events_; //其中 events_ 是它关心的 IO 事件,由用户设 置;
    int revents_;//revents_ 是目前活动的事件
    int index_; // used by poller


    EventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback errorCallback_;
};

}


#endif  // MUDUO_NET_CHANNEL_H