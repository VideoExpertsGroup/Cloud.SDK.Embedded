cmake_minimum_required(VERSION 2.8)

option(FLAG_USE_UBUNTU "Compile on Ubuntu)" OFF)

option(USE_FFSTREAMER_LIB "Use ffstreamer lib" OFF) 

option(DEFINE_HIKVISION "HIKVISION" OFF) 

IF(DEFINE_HIKVISION)
    ADD_DEFINITIONS(-DHIKVISION=1)
ENDIF(DEFINE_HIKVISION)

set(JANSSON_BUILD_DIR        lib CACHE PATH "Jansson build directory")
set(LIBWEBSOCKETS_BUILD_DIR  lib CACHE PATH "Libwebsockets build directory")
set(CLOUDSDK_BUILD_DIR       lib CACHE PATH "Libwebsockets build directory")

message(${JANSSON_BUILD_DIR})
message(${LIBWEBSOCKETS_BUILD_DIR})
message(${CLOUDSDK_BUILD_DIR})

if (USE_FFSTREAMER_LIB)
    message(AAAAAAAAAAAAAAAAA)
    message(${USE_FFSTREAMER_LIB})
    message(${FFSTREAMER_BUILD_DIR})
    set(FFSTREAMER_BUILD_DIR        lib CACHE PATH "Ffstreamer build directory")
    find_library(
                  ffstreamer
                  NAMES libffstreamer.a
                  NO_DEFAULT_PATH
                  HINTS "${FFSTREAMER_BUILD_DIR}/lib"
    )

endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)

#set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")

set (CMAKE_CXX_FLAGS "-std=c++11 -Os")

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
              NAMES libcloudsdk.a
              NO_DEFAULT_PATH
              HINTS "${CLOUDSDK_BUILD_DIR}/lib"
)

find_library(
              cloudsdk
              NAMES libffstreamer.a
              NO_DEFAULT_PATH
              HINTS "${CLOUDSDK_BUILD_DIR}/lib"
)

include_directories(${JANSSON_BUILD_DIR}/include
                    ${LIBWEBSOCKETS_BUILD_DIR}/include
                    ../src
                    ../src/cloud/core
)


# add the executable
add_executable(vxg_cloudstreamer test_cloudstreamer.cpp)

if (FLAG_USE_UBUNTU)
    message(${FLAG_USE_UBUNTU})
    target_link_libraries( # Specifies the target
                       vxg_cloudstreamer
                       # Links libraries to the target library.
    pthread
                       ${jansson}
                       ${libwebsockets}
                       ${cloudsdk}
    cap
)
else(USE_FFSTREAMER_LIB)

    message(XXXXXXXXXXXXXXXXX)
    message(${ffstreamer})

    target_compile_definitions(vxg_cloudstreamer PUBLIC USE_FFSTREAMER_LIB=1)

    target_link_libraries( # Specifies the target
                       vxg_cloudstreamer
                       # Links libraries to the target library.
    pthread
                       ${cloudsdk}
                       ${jansson}
                       ${libwebsockets}
                       ${ffstreamer}        
)
else()
    target_link_libraries( # Specifies the target
                       vxg_cloudstreamer
                       # Links libraries to the target library.
    pthread
                       ${cloudsdk}
                       ${jansson}
                       ${libwebsockets}
    #cap
)
endif()