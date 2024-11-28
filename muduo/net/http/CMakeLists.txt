set(http_SRCS
  HttpServer.cc
  HttpResponse.cc
  HttpContext.cc
  )

add_library(muduo_http ${http_SRCS})
target_link_libraries(muduo_http muduo_net)

install(TARGETS muduo_http DESTINATION lib)
set(HEADERS
  HttpContext.h
  HttpRequest.h
  HttpResponse.h
  HttpServer.h
  FileSingleton.h
  )
install(FILES ${HEADERS} DESTINATION include/muduo/net/http)


add_executable(httpserver_test test/HttpServer_test.cc)
target_link_libraries(httpserver_test muduo_http)
target_link_libraries(httpserver_test muduo_db)