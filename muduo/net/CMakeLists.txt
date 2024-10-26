set(net_SRCS
EventLoop.cc
)

add_library(muduo_net ${net_SRCS})
target_link_libraries(muduo_net muduo_base)

install(TARGETS muduo_net DESTINATION lib)

set(HEADERS
EventLoop.h
)

install(FILES ${HEADERS} DESTINATION include/muduo/net)

if(MUDUO_BUILD_EXAMPLES)
  message(STATUS "build tests")
  add_subdirectory(tests)
endif()