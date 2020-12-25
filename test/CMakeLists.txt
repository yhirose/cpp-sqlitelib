cmake_minimum_required(VERSION 3.14)
project(test)

add_executable(test-main test.cc sqlite3.c)

target_include_directories(test-main PRIVATE .. .)

enable_testing()

add_test(
  NAME TestMain
  COMMAND test-main
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
