set(net_SRCS
EventLoop.cc
Channel.cc
Poller.cc
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
EventLoop.h
Channel.h
Poller.h
Timer.h
TimerId.h
TimerQueue.h
Callback.h
EventLoopThread.h
Acceptor.h
InetAddress.h
Socket.h
SocketsOps.h
TcpConnection.h
TcpServer.h
Buffer.h
EventLoopThreadPool.h
Connector.h
TcpClient.h
)

install(FILES ${HEADERS} DESTINATION include/muduo/net)

if(MUDUO_BUILD_EXAMPLES)
  message(STATUS "build tests")
  add_subdirectory(tests)
endif()