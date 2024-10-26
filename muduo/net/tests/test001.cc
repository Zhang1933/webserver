#include "muduo/net/EventLoop.h"
#include "muduo/base/Thread.h"
#include <stdio.h>

void threadFunc()
{
  printf("threadFunc(): pid = %d, tid = %d\n",
         getpid(), muduo::CurrentThread::tid());

  muduo::net::EventLoop loop;
  loop.loop();
}

int main()
{
  printf("main(): pid = %d, tid = %d\n",
         getpid(), muduo::CurrentThread::tid());

  muduo::net::EventLoop loop;

  muduo::Thread thread(threadFunc);
  thread.start();

  loop.loop();
  pthread_exit(NULL);
}