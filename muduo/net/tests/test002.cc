#include "muduo/base/Logging.h"
#include "muduo/net/EventLoop.h"
#include "muduo/base/Thread.h"

muduo::net::EventLoop* g_loop;

void threadFunc()
{
  g_loop->loop();
}

int main()
{
  muduo::Logger::setLogLevel(muduo::Logger::TRACE);
  muduo::net::EventLoop loop;
  g_loop = &loop;
  muduo::Thread t(threadFunc);
  t.start();
  t.join();
}
