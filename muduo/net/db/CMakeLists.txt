set(db_SRCS
command.cpp  command_options.cpp  connection.cpp  connection_pool.cpp  errors.cpp  redis.cpp  reply.cpp
redis_uri.cpp
)

add_library(muduo_db ${db_SRCS})
target_link_libraries(muduo_db muduo_base)
target_link_libraries(muduo_db muduo_net)

# <------------ add hiredis dependency --------------->
find_path(HIREDIS_HEADER hiredis)
target_include_directories(muduo_db PUBLIC ${HIREDIS_HEADER})

find_library(HIREDIS_LIB hiredis)
target_link_libraries(muduo_db ${HIREDIS_LIB})

install(TARGETS muduo_db DESTINATION lib)

add_executable(redis_test test/redis_test.cc)
target_link_libraries(redis_test muduo_db)