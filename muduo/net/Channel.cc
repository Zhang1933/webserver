#include "Channel.h"
#include <cassert>
#include <poll.h>
#include <sys/poll.h>
#include "muduo/base/Logging.h"
#include "muduo/base/Timestamp.h"

using namespace muduo;

const int Channel::kReadEvent=POLLIN|POLLPRI;
const int Channel::kWriteEvent=POLLOUT;
const int Channel::kNoneEvent=0;

Channel::Channel(EventLoop *loop,int fdArg)
    :loop_(loop),
    fd_(fdArg),
    events_(0),
    revents_(0),
    index_(-1),
    eventHandling_(false)
{
}

Channel::~Channel()
{
    assert(!eventHandling_);//断言(assert())在事件处  理期间本 Channel 对象不会析构
}

void Channel::update()
{
    loop_->updateChannel(this);
}

void Channel::handleEvent(Timestamp receiveTime)
{
    eventHandling_=true;
    if(revents_&POLLNVAL){
        LOG_WARN << "Channel::handle_event() POLLNVAL";
    }
    if ((revents_ & POLLHUP) && !(revents_ & POLLIN)) {
    LOG_WARN << "Channel::handle_event() POLLHUP";
        if (closeCallback_) closeCallback_();
    }
    if(revents_&(POLLNVAL|POLLERR)){
        if(errorCallback_)errorCallback_();
    }
    if(revents_&(POLLIN|POLLPRI|POLLRDHUP)){
        if(readCallback_)readCallback_(receiveTime);
    }
    if(revents_&POLLOUT){
        if(writeCallback_)writeCallback_();
    }
    eventHandling_=false;
}