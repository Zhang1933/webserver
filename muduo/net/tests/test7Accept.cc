#include "muduo/net/Acceptor.h"
#include "muduo/net/EventLoop.h"
#include "muduo/net/EventLoopThread.h"
#include "muduo/net/InetAddress.h"
#include "muduo/net/SocketsOps.h"
#include <stdio.h>
#include <sys/types.h>

void newConnection(int sockfd, const muduo::InetAddress& peerAddr)
{
  printf("newConnection(): accepted a new connection from %s\n",
         peerAddr.toHostPort().c_str());
  ssize_t n=::write(sockfd, "How are you?\n", 13);
  (void)n;
  muduo::sockets::close(sockfd);
}

void newConnection2(int sockfd, const muduo::InetAddress& peerAddr)
{
  printf("newConnection2(): accepted a new connection from %s\n",
         peerAddr.toHostPort().c_str());
  ssize_t n=::write(sockfd, "go to the moon!\n", 17);
  (void)n;
  muduo::sockets::close(sockfd);
}


int main()
{
  printf("main(): pid = %d\n", getpid());

  muduo::InetAddress listenAddr(9981);
  muduo::EventLoop loop;

  muduo::Acceptor acceptor(&loop, listenAddr);
  acceptor.setNewConnectionCallback(newConnection);
  acceptor.listen();

  muduo::InetAddress listenAddr2(9982);
  muduo::Acceptor acceptor2(&loop, listenAddr2);
  acceptor2.setNewConnectionCallback(newConnection2);
  acceptor2.listen();

  loop.loop();
}
