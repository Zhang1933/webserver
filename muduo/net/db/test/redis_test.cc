#include "muduo/base/Logging.h"
#include <iostream>
#include <muduo/net/db/redis++.h>
#include <unistd.h>

using namespace sw::redis;

int main(){
    auto redis = Redis("tcp://root@127.0.0.1:6379&pool_size=1");
    muduo::Logger::setLogLevel(muduo::Logger::DEBUG);
    try {
        // Create an Redis object, which is movable but NOT copyable.
        

        // ***** STRING commands *****

        redis.set("key", "1234");
        auto val = redis.get("key");    // val is of type OptionalString. See 'API Reference' section for details.
        if (val) {
            // Dereference val to get the returned value of std::string type.
            std::cout <<"get reply:"<< *val << std::endl;
        }   // else key doesn't exist.

    } catch (const Error &e) {
        // Error handling.
        LOG_WARN<<e.what();
    }

    LOG_INFO<<"Test:start the redis,sleep 5s";
    sleep(5);
    try {
    // Create an Redis object, which is movable but NOT copyable.
    

    // ***** STRING commands *****

    redis.set("key", "1234");
    auto val = redis.get("key");    // val is of type OptionalString. See 'API Reference' section for details.
    if (val) {
        // Dereference val to get the returned value of std::string type.
        std::cout <<"get reply:"<< *val << std::endl;
    }   // else key doesn't exist.

    } catch (const Error &e) {
        // Error handling.
        LOG_WARN<<e.what();
    }
    // do something else
    LOG_INFO<<"Test success!";

}