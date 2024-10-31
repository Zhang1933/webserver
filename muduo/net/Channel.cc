#include "Channel.h"
#include <cassert>
#include <poll.h>
#include "muduo/base/Logging.h"
#include "muduo/base/Timestamp.h"
#include <sstream>


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
    eventHandling_(false),
    addedToLoop_(false)
{
}

Channel::~Channel()
{
    assert(!addedToLoop_);
    assert(!eventHandling_);//断言(assert())在事件处 理期间本 Channel 对象不会析构
}

void Channel::update()
{
    loop_->updateChannel(this);
}

void Channel::handleEvent(Timestamp receiveTime)
{
    eventHandling_=true;
    LOG_TRACE<<"handleEvent fd:"<<this->fd()<<" event"<<this->revents_;
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

void Channel::remove()
{
  assert(isNoneEvent());
  addedToLoop_ = false;
  loop_->removeChannel(this);
}

string Channel::reventsToString() const
{
  return eventsToString(fd_, revents_);
}

string Channel::eventsToString() const
{
  return eventsToString(fd_, events_);
}

string Channel::eventsToString(int fd, int ev)
{
  std::ostringstream oss;
  oss << fd << ": ";
  if (ev & POLLIN)
    oss << "IN ";
  if (ev & POLLPRI)
    oss << "PRI ";
  if (ev & POLLOUT)
    oss << "OUT ";
  if (ev & POLLHUP)
    oss << "HUP ";
  if (ev & POLLRDHUP)
    oss << "RDHUP ";
  if (ev & POLLERR)
    oss << "ERR ";
  if (ev & POLLNVAL)
    oss << "NVAL ";

  return oss.str();
}
