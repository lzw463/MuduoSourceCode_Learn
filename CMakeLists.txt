cmake_minimum_required(VERSION 3.0)
project(mymuduo)

# mymuduo最终编译成动态库，设置动态库的路径，放在根目录的lib文件下
set(LIBRARY_OUTPUT_DIRECTORY /usr/lib)
# 设置调试信息 以及 启动c++11标准（编译器新不需要）
set(CMAKE_CXX_FLAGS "$(CMAKE_CXX_FLAGS) -g -std=c++11")

# 定义参与编译的源代码文件
aux_source_directory(. SRC_LIST)
# 编译动态库
add_library(mymuduo SHARED $(SRC_LIST))