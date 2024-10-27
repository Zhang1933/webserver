#ifndef MUDUO_NET_TIMERID_H
#define MUDUO_NET_TIMERID_H

#include "muduo/base/copyable.h"
#include "muduo/net/Timer.h"

namespace muduo {

///
/// An opaque identifier, for canceling Timer.
///
class TimerId:muduo::copyable{
public:
    explicit TimerId(Timer* timer)
    :value_(timer)
    {
    }
    // default copy-ctor, dtor and assignment are okay

private:
    Timer* value_;
};
}

#endif