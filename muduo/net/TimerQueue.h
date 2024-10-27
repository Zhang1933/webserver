#ifndef MUDUO_NET_TIMERQUEUE_H
#define MUDUO_NET_TIMERQUEUE_H



#include "muduo/base/Timestamp.h"
#include "muduo/base/noncopyable.h"
#include "muduo/net/Timer.h"
#include "muduo/net/TimerId.h"
#include "muduo/net/Callback.h"
#include "muduo/net/Channel.h"
#include <memory>
#include <set>
#include <utility>
#include <vector>

namespace muduo
{

class EventLoop;
///
/// A best efforts timer queue.
/// No guarantee that the callback will be on time.
///
class TimerQueue:noncopyable
{
public:
    TimerQueue(EventLoop* loop);
    ~TimerQueue();

    ///
    /// Schedules the callback to be run at given time,
    /// repeats if @c interval > 0.0.
    ///
    /// Must be thread safe. Usually be called from other threads.
    TimerId addTimer(const TimerCallback&cb,
    Timestamp when,
    double interval);



    // void cancel(TimerId timerId);
private:
    typedef std::pair<Timestamp, Timer*> Entry;
    typedef std::set<Entry> TimerList;

    bool insert(Timer* timer);
    
    void addTimerInLoop(Timer* timer);
    // called when timerfd alarms
    void handleRead();
    void reset(std::vector<Entry>& expired, Timestamp now);

    EventLoop* loop_;
   
    // move out all expired timers
    std::vector<Entry>getExpired(Timestamp now);

    const int timerfd_;
    Channel timerfdChannel_;
     // Timer list sorted by expiration
    TimerList timers_;
};
}
#endif  // MUDUO_NET_TIMERQUEUE_H
