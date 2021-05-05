cmake_minimum_required(VERSION 2.8)
project(ljrServer)

include (cmake/utils.cmake)

# 显示执行构建过程中详细的信息
set(CMAKE_VERBOSE_MAKEFILE ON)

# 包含路径
include_directories(.)

# 版本1
# set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -g -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function")
# gdb调试版本，生成的程序可以用vscode调试
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined")

set(LIB_SRC
    ljrServer/address.cpp
    ljrServer/bytearray.cpp
    ljrServer/config.cpp
    ljrServer/fiber.cpp
    ljrServer/fd_manager.cpp
    ljrServer/hook.cpp
    ljrServer/http/http.cpp
    ljrServer/http/http_parser.cpp
    ljrServer/http/http_connection.cpp
    ljrServer/http/http_session.cpp
    ljrServer/http/http_server.cpp
    ljrServer/http/http11_parser.cpp
    ljrServer/http/httpclient_parser.cpp
    ljrServer/http/servlet.cpp;
    ljrServer/iomanager.cpp
    ljrServer/log.cpp
    ljrServer/scheduler.cpp
    ljrServer/socket.cpp
    ljrServer/socket_stream.cpp
    ljrServer/stream.cpp
    ljrServer/thread.cpp
    ljrServer/tcp_server.cpp
    ljrServer/timer.cpp
    ljrServer/util.cpp
)

# 编译 ragel
ragelmaker(ljrServer/http/http11_parser.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/ljrServer/http)
ragelmaker(ljrServer/http/httpclient_parser.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/ljrServer/http)
ragelmaker(ljrServer/uri.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/ljrServer)

# 生成so文件
add_library(ljrServer SHARED ${LIB_SRC})

# 重定义宏__FILE__
force_redefine_file_macro_for_sources(ljrServer)

set(LIBS
    ljrServer
    dl
    pthread
    yaml-cpp
)

# message("******", ${LIB_LIB})

# 测试日志
add_executable(test tests/test.cpp)
add_dependencies(test ljrServer)
# 重定义宏__FILE__
force_redefine_file_macro_for_sources(test)
target_link_libraries(test ${LIBS})

# 测试配置
add_executable(test_config tests/test_config.cpp)
add_dependencies(test_config ljrServer)
# 重定义宏__FILE__
force_redefine_file_macro_for_sources(test_config)
target_link_libraries(test_config ${LIBS})

# 测试线程
add_executable(test_thread tests/test_thread.cpp)
add_dependencies(test_thread ljrServer)
# 重定义宏__FILE__
force_redefine_file_macro_for_sources(test_thread)
target_link_libraries(test_thread ${LIBS})

# 测试宏
add_executable(test_util tests/test_util.cpp)
add_dependencies(test_util ljrServer)
# 重定义宏__FILE__
force_redefine_file_macro_for_sources(test_util)
target_link_libraries(test_util ${LIBS})

# 测试协程
add_executable(test_fiber tests/test_fiber.cpp)
add_dependencies(test_fiber ljrServer)
# 重定义宏__FILE__
force_redefine_file_macro_for_sources(test_fiber)
target_link_libraries(test_fiber ${LIBS})

# 测试协程调度
add_executable(test_scheduler tests/test_scheduler.cpp)
add_dependencies(test_scheduler ljrServer)
# 重定义宏__FILE__
force_redefine_file_macro_for_sources(test_scheduler)
target_link_libraries(test_scheduler ${LIBS})

# 测试IO协程调度
add_executable(test_iomanager tests/test_iomanager.cpp)
add_dependencies(test_iomanager ljrServer)
# 重定义宏__FILE__
force_redefine_file_macro_for_sources(test_iomanager)
target_link_libraries(test_iomanager ${LIBS})

# 测试 HOOK
add_executable(test_hook tests/test_hook.cpp)
add_dependencies(test_hook ljrServer)
# 重定义宏__FILE__
force_redefine_file_macro_for_sources(test_hook)
target_link_libraries(test_hook ${LIBS})

# 测试 address
add_executable(test_address tests/test_address.cpp)
add_dependencies(test_address ljrServer)
# 重定义宏__FILE__
force_redefine_file_macro_for_sources(test_address)
target_link_libraries(test_address ${LIBS})

# 测试 socket
add_executable(test_socket tests/test_socket.cpp)
add_dependencies(test_socket ljrServer)
# 重定义宏__FILE__
force_redefine_file_macro_for_sources(test_socket)
target_link_libraries(test_socket ${LIBS})

# 测试 bytearray
add_executable(test_bytearray tests/test_bytearray.cpp)
add_dependencies(test_bytearray ljrServer)
# 重定义宏__FILE__
force_redefine_file_macro_for_sources(test_bytearray)
target_link_libraries(test_bytearray ${LIBS})

# 测试 http
add_executable(test_http tests/test_http.cpp)
add_dependencies(test_http ljrServer)
# 重定义宏__FILE__
force_redefine_file_macro_for_sources(test_http)
target_link_libraries(test_http ${LIBS})

# 测试 http_parser
add_executable(test_http_parser tests/test_http_parser.cpp)
add_dependencies(test_http_parser ljrServer)
# 重定义宏__FILE__
force_redefine_file_macro_for_sources(test_http_parser)
target_link_libraries(test_http_parser ${LIBS})

# 测试 tcp_server
add_executable(test_tcp_server tests/test_tcp_server.cpp)
add_dependencies(test_tcp_server ljrServer)
# 重定义宏__FILE__
force_redefine_file_macro_for_sources(test_tcp_server)
target_link_libraries(test_tcp_server ${LIBS})

# 实例 echo_server
add_executable(echo_server examples/echo_server.cpp)
add_dependencies(echo_server ljrServer)
# 重定义宏__FILE__
force_redefine_file_macro_for_sources(echo_server)
target_link_libraries(echo_server ${LIBS})

# 测试 http_server
ljrserver_add_executable(test_http_server "tests/test_http_server.cpp" ljrServer "${LIBS}")

# 测试 http_connection
ljrserver_add_executable(test_http_connection "tests/test_http_connection.cpp" ljrServer "${LIBS}")

# 测试 uri
ljrserver_add_executable(test_uri "tests/test_uri.cpp" ljrServer "${LIBS}")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

