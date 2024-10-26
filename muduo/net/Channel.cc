#include "Channel.h"
#include <poll.h>
#include <sys/poll.h>
#include "muduo/base/Logging.h"

using namespace muduo;

const int Channel::kReadEvent=POLLIN|POLLPRI;
const int Channel::kWriteEvent=POLLOUT;
const int Channel::kNoneEvent=0;

Channel::Channel(EventLoop *loop,int fdArg)
    :loop_(loop),
    fd_(fdArg),
    events_(0),
    revents_(0),
    index_(-1)
{
}

void Channel::update()
{
    loop_->updateChannel(this);
}

void Channel::handleEvent()
{
    if(revents_&POLLNVAL){
        LOG_WARN << "Channel::handle_event() POLLNVAL";
    }
    if(revents_&(POLLNVAL|POLLERR)){
        if(errorCallback_)errorCallback_();
    }
    if(revents_&(POLLIN|POLLPRI|POLLRDHUP)){
        if(readCallback_)readCallback_();
    }
    if(revents_&POLLOUT){
        if(writeCallback_)writeCallback_();
    }
}