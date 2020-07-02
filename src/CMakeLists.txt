cmake_minimum_required(VERSION 2.8)

project(cloudsdk)

set(JANSSON_BUILD_DIR        lib CACHE PATH "Jansson build directory")
set(LIBWEBSOCKETS_BUILD_DIR  lib CACHE PATH "Libwebsockets build directory")
set(OPENSSL_BUILD_DIR        lib CACHE PATH "openssl build directory")
set(MBEDTLS_BUILD_DIR        lib CACHE PATH "mbedtls build directory")

option(HUGE_LOG "Huge log file" OFF)
option(OLDCPP "Old cpp version, less than cpp11" OFF)
option(WITH_OPENSSL "build with openssl library" OFF)
option(WITH_MBEDTLS "build with openssl library" OFF)
option(DISABLE_RTMPS "Disable rtmps protocol" OFF)


message(${JANSSON_BUILD_DIR})
message(${LIBWEBSOCKETS_BUILD_DIR})
message(${OPENSSL_BUILD_DIR})
message(${MBEDTLS_BUILD_DIR})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)

#set (CMAKE_CXX_FLAGS "-std=c++11 -fPIC -Os")

if (CMAKE_HOST_WIN32)
	set(CMAKE_CXX_FLAGS "-std=c++11 -Os /EHsc")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
else()
	if(OLDCPP)
#		set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
		set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -fPIC -O2")
	else()
		set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fPIC -O2 -Wno-psabi")
	endif()
endif()


include_directories(${JANSSON_BUILD_DIR}/include
                    ${LIBWEBSOCKETS_BUILD_DIR}/include
)

if (WITH_MBEDTLS)
	include_directories(${JANSSON_BUILD_DIR}/include
                            ${LIBWEBSOCKETS_BUILD_DIR}/include
                            ${MBEDTLS_BUILD_DIR}/include
)
endif()

if(WITH_OPENSSL)
	include_directories(${JANSSON_BUILD_DIR}/include
                            ${LIBWEBSOCKETS_BUILD_DIR}/include
	                    ${OPENSSL_BUILD_DIR}/include
)
endif()

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

add_library( # Specifies the name of the library.
          cloudsdk

          # Sets the library as a shared library.
          STATIC

          # Provides a relative path to your source file(s).
	    Enums/CloudReturnCodes.h

	    cloud/core/CameraManager/CameraManager.cpp
	    cloud/core/CameraManager/CameraManager.h
	    cloud/core/CameraManager/CameraManagerByeReasons.h
	    cloud/core/CameraManager/CameraManagerCommands.h
	    cloud/core/CameraManager/CameraManagerConfig.h
	    cloud/core/CameraManager/CameraManagerDoneStatus.h
	    cloud/core/CameraManager/CameraManagerParams.h
	    cloud/core/CameraManager/ICameraManagerCallback.h
	    cloud/core/CameraManager/StreamConfig.h
	    cloud/core/CameraManager/StreamConfigAudio.h
	    cloud/core/CameraManager/StreamConfigVideo.h
	    cloud/core/CameraManager/CameraEvent.h

	    cloud/core/CloudCommon/CloudRegToken.h
	    cloud/core/CloudCommon/CloudToken.h

	    cloud/core/CloudAPI.h
	    cloud/core/CloudAPIEndPoints.h
	    cloud/core/CloudHelpers.h
	    cloud/core/Uri.h

	    Interfaces/ICloudStreamerCallback.h

	    Objects/CloudShareConnection.h

	    utils/_cunk.h
	    utils/base64.c
	    utils/base64.h
	    utils/Context.h
	    utils/MLog.cpp
	    utils/MLog.h
	    utils/utils.cpp
	    utils/utils.h
	    utils/windefsws.cpp
	    utils/windefsws.h
	    utils/wswrap.cpp
	    utils/wswrap.h
	    utils/FileRing.cpp
	    utils/FileRing.h
	    CloudSDK.h
	    CloudStreamerSDK.cpp
	    CloudStreamerSDK.h
)

if(HUGE_LOG)
    target_compile_definitions(cloudsdk PUBLIC HUGE_LOG=1)
endif()

if(OLDCPP)
    target_compile_definitions(cloudsdk PUBLIC OLDCPP=1)
endif()

if(WITH_OPENSSL)
    target_compile_definitions(cloudsdk PUBLIC WITH_OPENSSL=1)
endif()

if (WITH_MBEDTLS)
    target_compile_definitions(cloudsdk PUBLIC WITH_MBEDTLS=1)
endif()

if (DISABLE_RTMPS)
    target_compile_definitions(cloudsdk PUBLIC DISABLE_RTMPS=1)
endif()

if (CMAKE_HOST_WIN32)
    target_compile_definitions(cloudsdk PUBLIC WIN32=1)
    target_compile_definitions(cloudsdk PUBLIC _WIN32=1)
    target_compile_definitions(cloudsdk PUBLIC WIN32_LEAN_AND_MEAN=1)
endif()
