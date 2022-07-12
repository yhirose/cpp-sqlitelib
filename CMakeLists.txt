cmake_minimum_required(VERSION 3.14)
project(cpp-sqlitelib)

set(CMAKE_CXX_STANDARD 17)

if(MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:__cplusplus /utf-8")
endif()

add_library(cpp-sqlitelib INTERFACE)
target_include_directories(cpp-sqlitelib INTERFACE .)
target_link_libraries(cpp-sqlitelib INTERFACE sqlite3)
