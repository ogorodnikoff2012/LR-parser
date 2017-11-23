cmake_minimum_required(VERSION 3.7)
project(LR1)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "-g -DDEBUG -Wall -Wextra -Werror")

set(SOURCE_FILES Grammar.h CanonicalLR.h misc.cpp)
add_executable(LR1 main.cpp ${SOURCE_FILES})
add_executable(hometask hometask.cpp ${SOURCE_FILES})

