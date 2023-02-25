cmake_minimum_required(VERSION 3.6)

set(CMAKE_BUILD_TYPE RELEASE)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}" ${CMAKE_MODULE_PATH})
project(libcamera-demo)
find_package(PkgConfig REQUIRED)
pkg_check_modules(CAMERA REQUIRED libcamera)

find_library(LIBCAMERA_LIBRARY libcamera.so REQUIRED)
find_library(LIBCAMERA_BASE_LIBRARY libcamera-base.so REQUIRED)
message(STATUS ${CAMERA_INCLUDE_DIRS})
message(STATUS ${LIBCAMERA_LIBRARY})
message(STATUS ${LIBCAMERA_BASE_LIBRARY})

include_directories(. "${CAMERA_INCLUDE_DIRS}")
set(LIBCAMERA_LIBRARIES "${LIBCAMERA_LIBRARY}" "${LIBCAMERA_BASE_LIBRARY}")
add_executable(libcamera-demo main.cpp LibCamera.cpp network.cpp lz4.c)

target_link_libraries(libcamera-demo "${LIBCAMERA_LIBRARIES}" pigpio)

message("Release flags: " ${CMAKE_CXX_FLAGS_RELEASE})
message("Debug flags: " ${CMAKE_CXX_FLAGS_DEBUG})
message("Build type: " ${CMAKE_BUILD_TYPE})