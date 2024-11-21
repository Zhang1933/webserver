#include <iostream>
#include <muduo/net/db/redis++.h>

using namespace sw::redis;

int main(){
try {
    // Create an Redis object, which is movable but NOT copyable.
    auto redis = Redis("tcp://root@127.0.0.1:6379&pool_size=10");

    // ***** STRING commands *****

    redis.set("key", "1234");
    auto val = redis.get("key");    // val is of type OptionalString. See 'API Reference' section for details.
    if (val) {
        // Dereference val to get the returned value of std::string type.
        std::cout << *val << std::endl;
    }   // else key doesn't exist.


} catch (const Error &e) {
    // Error handling.
}
}