cmake_minimum_required(VERSION 2.8)
project(ljrServer)

# 显示执行构建过程中详细的信息
set(CMAKE_VERBOSE_MAKEFILE ON)

# 包含路径
include_directories(.)

# 版本1
# set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -g -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function")
# gdb调试版本，生成的程序可以用vscode调试
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function")

set(LIB_SRC
    ljrServer/log.cpp
    ljrServer/util.cpp
    # ljrServer/singleton.cpp
    ljrServer/config.cpp
)

# 生成so文件
add_library(ljrServer SHARED ${LIB_SRC})

add_executable(test tests/test.cpp)
add_dependencies(test ljrServer)
target_link_libraries(test ljrServer yaml-cpp)

add_executable(test_config tests/test_config.cpp)
add_dependencies(test_config ljrServer)
target_link_libraries(test_config ljrServer yaml-cpp)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

