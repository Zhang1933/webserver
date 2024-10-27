#include "muduo/net/Timer.h"
#include "muduo/base/Logging.h"
#include "muduo/base/Timestamp.h"

using namespace muduo;
void Timer::restart(Timestamp now)
{
    if(repeat_)
    {
        expiration_=addTime(now, interval_);
    }
    else 
    {
        LOG_WARN<<"restart,Timer repeat is not true";
        expiration_=Timestamp::invalid();    
    }
}