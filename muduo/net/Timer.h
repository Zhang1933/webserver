#ifndef MUDUO_NET_TIMER_H
#define MUDUO_NET_TIMER_H


#include "muduo/base/Timestamp.h"
#include "muduo/base/noncopyable.h"
#include "muduo/net/Callback.h"
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
    repeat_(interval>0.0)
    { }

    void run()const
    {
        callback_();
    }

    bool repeat()const{return repeat_;}
    Timestamp expireation()const {return expiration_;}
    void restart(Timestamp now);

private:
    const TimerCallback callback_;
    Timestamp expiration_;
    const double interval_;
    const bool repeat_;
};
}

#endif