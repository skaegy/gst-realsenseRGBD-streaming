cmake_minimum_required(VERSION 3.12)
project(RGBDStreaming)

set(CMAKE_CXX_STANDARD 11)

IF(NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE Debug)
ENDIF()

MESSAGE("Build type: " ${CMAKE_BUILD_TYPE})

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  -Wall -O0 -g  -march=native ")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O0 -g -march=native")

# Check C++11 or C++0x support
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    add_definitions(-DCOMPILEDWITHC11)
    message(STATUS "Using flag -std=c++11.")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
    add_definitions(-DCOMPILEDWITHC0X)
    message(STATUS "Using flag -std=c++0x.")
else()
    message(FATAL_ERROR "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

set(OpenCV_DIR /opt/ros/kinetic/share/OpenCV-3.3.1-dev) # LINK TO OPENCV IN ROS
find_package(OpenCV REQUIRED)
find_package(realsense2 REQUIRED)
find_package(PkgConfig)

pkg_check_modules(GST REQUIRED gstreamer-1.0>=1.4
        gstreamer-sdp-1.0>=1.4
        gstreamer-video-1.0>=1.4
        gstreamer-app-1.0>=1.4
        gstreamer-rtsp-server-1.0>=1.4)

include_directories(
${OPENCV_INCLUDE_DIR}
${realsense2_INCLUDE_DIRS}
${GST_INCLUDE_DIRS}
)

add_executable(RGBDStreaming RGBDstreaming.cpp)
target_link_libraries(RGBDStreaming
        ${OpenCV_LIBS}
        ${realsense2_LIBRARIES}
        ${GST_LIBRARIES}
        realsense2
        )

add_executable(UDPserver UDPserver.cpp)

add_executable(UDPlistener UDPlistener.cpp)