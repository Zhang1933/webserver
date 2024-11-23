#include "muduo/base/Logging.h"
#include "muduo/net/db/redis.h"
#include <cstdio>
#include <iostream>
#include <muduo/net/db/redis++.h>
#include <unistd.h>

using namespace sw::redis;
Redis redis = Redis("tcp://root@127.0.0.1:6379?pool_size=3");

int main(){
    muduo::Logger::setLogLevel(muduo::Logger::DEBUG);

    

    int cnt=4;
    for(int i=0;i<cnt;i++){
        if(i==cnt-1){
            printf("last try,sleep 5\n");
            sleep(5);
        }
        try {
            // Create an Redis object, which is movable but NOT copyable.
            printf("try %d\n",i);

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
    }
    
    // do something else
    printf("sleep 10");
    redis.get("nonexistkey");
    LOG_INFO<<"Test success!";

}