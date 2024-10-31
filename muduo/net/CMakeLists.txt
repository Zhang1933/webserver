set(net_SRCS
EventLoop.cc
Channel.cc
Poller.cc
poller/DefaultPoller.cc
poller/EPollPoller.cc
poller/PollPoller.cc
Timer.cc
TimerId.cc
TimerQueue.cc
EventLoopThread.cc
Acceptor.cc
InetAddress.cc
Socket.cc
SocketsOps.cc
TcpConnection.cc
TcpServer.cc
Buffer.cc
EventLoopThreadPool.cc
Connector.cc
TcpClient.cc
)

add_library(muduo_net  ${net_SRCS})
target_link_libraries(muduo_net muduo_base)

install(TARGETS muduo_net DESTINATION lib)


set(HEADERS
  Buffer.h
  Callbacks.h
  Channel.h
  EventLoop.h
  EventLoopThread.h
  EventLoopThreadPool.h
  InetAddress.h
  TcpClient.h
  TcpConnection.h
  TcpServer.h
  TimerId.h
  )

install(FILES ${HEADERS} DESTINATION include/muduo/net)

if(MUDUO_BUILD_EXAMPLES)
  message(STATUS "build tests")
  add_subdirectory(tests)
endif()