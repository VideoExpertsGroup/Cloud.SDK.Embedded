cmake_minimum_required(VERSION 2.8)

option(FLAG_USE_UBUNTU "Compile on Ubuntu)" OFF)

set(JANSSON_BUILD_DIR        lib CACHE PATH "Jansson build directory")
set(LIBWEBSOCKETS_BUILD_DIR  lib CACHE PATH "Libwebsockets build directory")
set(CLOUDSDK_BUILD_DIR       lib CACHE PATH "Libwebsockets build directory")

message(${JANSSON_BUILD_DIR})
message(${LIBWEBSOCKETS_BUILD_DIR})
message(${CLOUDSDK_BUILD_DIR})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)

#set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")

set (CMAKE_CXX_FLAGS "-std=c++11")

find_library(
              jansson
              NAMES libjansson.a
              NO_DEFAULT_PATH
              HINTS "${JANSSON_BUILD_DIR}/lib"
)

find_library(
              libwebsockets
              NAMES libwebsockets.a
              NO_DEFAULT_PATH
              HINTS "${LIBWEBSOCKETS_BUILD_DIR}/lib"
)

find_library(
              cloudsdk
              NAMES libcloudsdk.so
              NO_DEFAULT_PATH
              HINTS "${CLOUDSDK_BUILD_DIR}/lib"
)

include_directories(${JANSSON_BUILD_DIR}/include
                    ${LIBWEBSOCKETS_BUILD_DIR}/include
                    ../src
                    ../src/cloud/core
)


# add the executable
add_executable(test_cloudstreamer test_cloudstreamer.cpp)

if (FLAG_USE_UBUNTU)
#    message(${FLAG_USE_UBUNTU})
    target_link_libraries( # Specifies the target
                       test_cloudstreamer
                       # Links libraries to the target library.
    pthread
    #                      ${jansson}
    #                      ${libwebsockets}
                       ${cloudsdk}
    cap
)
else()
    target_link_libraries( # Specifies the target
                       test_cloudstreamer
                       # Links libraries to the target library.
    pthread
    #                      ${jansson}
    #                      ${libwebsockets}
                       ${cloudsdk}
    #cap
)
endif()