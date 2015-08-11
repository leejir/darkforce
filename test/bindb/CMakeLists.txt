PROJECT(bindb)
CMAKE_MINIMUM_REQUIRED(VERSION 2.6) 

file(GLOB logic_head_file ./ ./*.h)

set(logic_src
    ./main.cpp
)
add_executable(bindb ${logic_src} ${logic_head_file})