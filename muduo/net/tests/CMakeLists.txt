# 获取当前目录中的所有 .cc 文件
file(GLOB TEST_SOURCES  "${CMAKE_CURRENT_SOURCE_DIR}/*.cc")

foreach(test ${TEST_SOURCES})
    # 从文件名生成目标名
    get_filename_component(test_name ${test} NAME_WE)
    set(test_target "reactor_${test_name}")

    # 添加可执行文件
    add_executable(${test_target} ${test})

    # 链接库
    target_link_libraries(${test_target} muduo_net)
endforeach()