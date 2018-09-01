cmake_minimum_required(VERSION 3.5)
project(NetworkProtocolTest)

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
find_package(Boost  1.54.0  	     # Minimum or EXACT version e.g. 1.36.0
        REQUIRED             					# Fail with error if Boost is not found
        COMPONENTS serialization 				# Boost libraries by their canonical name
        )
pkg_check_modules(GST REQUIRED
        gstreamer-1.0>=1.4
        gstreamer-sdp-1.0>=1.4
        gstreamer-video-1.0>=1.4
        gstreamer-app-1.0>=1.4
        gstreamer-rtsp-server-1.0>=1.4)
pkg_check_modules(GLIB REQUIRED glib-2.0>=2.23)

include_directories(
${PROJECT_SOURCE_DIR}
${OPENCV_INCLUDE_DIR}
${realsense2_INCLUDE_DIRS}
${GST_INCLUDE_DIRS}
${GLIB_INCLUDE_DIRS}
)

add_library(${PROJECT_NAME} SHARED
udpSocketSimple.h
udpSocketSimple.cpp)

add_executable(RGBDStreaming RGBDstreaming.cpp)
target_link_libraries(RGBDStreaming
        ${OpenCV_LIBS}
        ${realsense2_LIBRARIES}
        ${GST_LIBRARIES}
        realsense2
        )

add_executable(RGBD_rtsp_streaming RGBD_rtsp_streaming.cpp)
target_link_libraries(RGBD_rtsp_streaming
        ${OpenCV_LIBS}
        ${realsense2_LIBRARIES}
        ${GST_LIBRARIES}
        ${GLIB_LDFLAGS}
        realsense2
        )

add_executable(UDPserver UDPserver.cpp)
target_link_libraries(UDPserver
        ${OpenCV_LIBS})

add_executable(UDPlistener UDPlistener.cpp)
target_link_libraries(UDPlistener
        ${OpenCV_LIBS})

add_executable(udpSocketTest udpSocketTest.cpp)
target_link_libraries(udpSocketTest
        ${PROJECT_NAME}
        ${OpenCV_LIBS}
        ${Boost_LIBRARIES}
        pthread)