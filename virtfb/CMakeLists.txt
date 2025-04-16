cmake_minimum_required(VERSION 3.16)
project(virtfb)

find_package(OpenCV REQUIRED)

add_executable(virtfb virtfb.cpp)
target_include_directories(virtfb PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(virtfb PRIVATE ${OpenCV_LIBS})

