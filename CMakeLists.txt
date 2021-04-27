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
    ljrServer/log.cpp
    ljrServer/util.cpp
    ljrServer/config.cpp
    ljrServer/thread.cpp
)

# 生成so文件
add_library(ljrServer SHARED ${LIB_SRC})

# 重定义宏__FILE__
force_redefine_file_macro_for_sources(ljrServer)

set(LIBS
    ljrServer
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

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

