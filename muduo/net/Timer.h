#ifndef MUDUO_NET_TIMER_H
#define MUDUO_NET_TIMER_H


#include "muduo/base/Timestamp.h"
#include "muduo/base/noncopyable.h"
#include "muduo/net/Callback.h"
#include "muduo/base/Atomic.h"

namespace muduo {
 
///
/// Internal class for timer event.
///
class Timer:noncopyable{
public:
    Timer(const TimerCallback&cb,Timestamp when,double interval)
    :callback_(cb),
    expiration_(when),
    interval_(interval),
    repeat_(interval>0.0),
    sequence_(s_numCreated_.incrementAndGet())
    { }

    void run()const
    {
        callback_();
    }

    bool repeat()const{return repeat_;}
    Timestamp expiration()const {return expiration_;}
    void restart(Timestamp now);
    int64_t sequence() const { return sequence_; }


private:
    const TimerCallback callback_;
    Timestamp expiration_;
    const double interval_;
    const bool repeat_;
    const int64_t sequence_;

    static AtomicInt64 s_numCreated_;
};
}

#endif