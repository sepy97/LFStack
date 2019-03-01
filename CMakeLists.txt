cmake_minimum_required(VERSION 3.13)
project(LFStack)

set(CMAKE_CXX_STANDARD 14)

include_directories(/usr/local/include)
LINK_DIRECTORIES( /usr/local/lib/)

find_library(CDS libcds.dylib)

add_executable(LFStack main.cpp)

target_link_libraries(LFStack CDS)