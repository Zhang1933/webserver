#include "muduo/net/Poller.h"
#include "muduo/net/poller/PollPoller.h"
#include "muduo/net/poller/EPollPoller.h"

#include <stdlib.h>

using namespace muduo;

Poller* Poller::newDefaultPoller(EventLoop* loop)
{
  //FIXME:水平触发，边缘触发不兼容
  // if (::getenv("MUDUO_USE_POLL"))
  // {
  //   return new PollPoller(loop);
  // }
  // else
  // {
    return new EPollPoller(loop);
  //}
}
