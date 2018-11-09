cmake_minimum_required(VERSION 2.8.9)
project(interface)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/lib")

set(CMAKE_CXX_FLAGS "-O2 -g -Wall -std=c++11")
set(CMAKE_C_FLAGS "-O2 -g -Wall -std=c11")

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

include(use-qt5 )
include(use-doxygen)
include(qt-add-executable)
include(qt-add-library)

include_directories(src)
include_directories(include)

#add_subdirectory(src)
