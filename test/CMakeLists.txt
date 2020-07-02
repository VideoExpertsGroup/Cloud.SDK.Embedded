cmake_minimum_required(VERSION 2.8)

project(test_cloudstreamer)

option(FLAG_USE_UBUNTU "Compile on Ubuntu)" OFF)
option(NOEXECINFOH "Toolchain is not supports <execinfo.h>" OFF)
option(OLDCPP "Old cpp version, less than cpp11" OFF)
option(WITH_OPENSSL "build with openssl library" OFF)

if (CMAKE_HOST_WIN32)
    set (STATICLIBEXT "lib")
    set (SHAREDLIBEXT "dll")
    set (LIBPREFIX "")
    if(CMAKE_BUILD_TYPE MATCHES Release)
        set (LIBFINDSUFFIX "Release")
        set (LIBPOSTFIX "")
        set (LIBPOSTFIX2 "")
        set (MSVCRTLIB "MSVCRT.lib")
    else()
        set (LIBFINDSUFFIX "Debug")
        set (LIBPOSTFIX "_d")
        set (LIBPOSTFIX2 "-d")
        set (MSVCRTLIB "MSVCRTd.lib")
    endif()
else()
    set (STATICLIBEXT "a")
    set (SHAREDLIBEXT "so")
    set (LIBPREFIX "lib")
    set (LIBFINDSUFFIX "")
    set (LIBPOSTFIX "")
    set (LIBPOSTFIX2 "")
endif()

set(JANSSON_BUILD_DIR        lib CACHE PATH "Jansson build directory")
set(LIBWEBSOCKETS_BUILD_DIR  lib CACHE PATH "Libwebsockets build directory")
set(CLOUDSDK_BUILD_DIR       lib CACHE PATH "Cloudsdk build directory")
set(OPENSSL_BUILD_DIR        lib CACHE PATH "openssl build directory")
set(CURL_SOURCE_DIR          lib CACHE PATH "CURL source directory")

message("-------------------------------------------------------")
message("JANSSON_BUILD_DIR=" ${JANSSON_BUILD_DIR})
message("LIBWEBSOCKETS_BUILD_DIR=" ${LIBWEBSOCKETS_BUILD_DIR})
message("CLOUDSDK_BUILD_DIR=" ${CLOUDSDK_BUILD_DIR})
message("FLAG_USE_UBUNTU=" ${FLAG_USE_UBUNTU})
message("OPENSSL_BUILD_DIR=" ${OPENSSL_BUILD_DIR})
message("WITH_OPENSSL=" ${WITH_OPENSSL})
message("-------------------------------------------------------")


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)

#set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")

if (CMAKE_HOST_WIN32)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Os /EHsc /MTd")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
else()
#	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fPIC -O2 -Wno-psabi")
	if(OLDCPP)
		set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
	else()
		set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fPIC -O2 -Wno-psabi")
	endif()
endif()

find_library(
              jansson
              NAMES ${LIBPREFIX}jansson${LIBPOSTFIX}.${STATICLIBEXT}
              NO_DEFAULT_PATH
              HINTS "${JANSSON_BUILD_DIR}/lib/${LIBFINDSUFFIX}"
)

if (CMAKE_HOST_WIN32)

	find_library(
              libwebsockets_static
              NAMES ${LIBPREFIX}websockets_static.${STATICLIBEXT}
              NO_DEFAULT_PATH
              HINTS "${LIBWEBSOCKETS_BUILD_DIR}/lib/${LIBFINDSUFFIX}"
)
else()

	find_library(
              libwebsockets
              NAMES ${LIBPREFIX}websockets.${STATICLIBEXT}
              NO_DEFAULT_PATH
              HINTS "${LIBWEBSOCKETS_BUILD_DIR}/lib/${LIBFINDSUFFIX}"
)
endif()

find_library(
              cloudsdk
              NAMES ${LIBPREFIX}cloudsdk.${STATICLIBEXT}
              NO_DEFAULT_PATH
              HINTS "${CLOUDSDK_BUILD_DIR}/lib/${LIBFINDSUFFIX}"
)

if(WITH_OPENSSL)
    find_library(                 
             libssl
             NAMES libssl.${STATICLIBEXT}
             NO_DEFAULT_PATH
             HINTS "${OPENSSL_BUILD_DIR}/lib"
    )
    find_library(                 
             libcrypto
             NAMES libcrypto.${STATICLIBEXT}
             NO_DEFAULT_PATH
             HINTS "${OPENSSL_BUILD_DIR}/lib"
    )
endif()

	include_directories(${JANSSON_BUILD_DIR}/include
                    ../external_libs/cctz/include
                    ${LIBWEBSOCKETS_BUILD_DIR}/include
		    ${XML_BUILD_DIR}/include/libxml2
                    ${OPENSSL_BUILD_DIR}/include/
                    ../external_libs/libxml2-2.9.7/include 
		    ../external_libs/${CURL_SOURCE_DIR}/include
                    ../src
                    ../src/cloud/core
		    ../src/utils
                    ${FFMPEG_INC_DIR}
		    Interfaces
		)


add_executable(test_cloudstreamer test_cloudstreamer.cpp )

if(FLAG_USE_UBUNTU)
target_compile_definitions(test_cloudstreamer PUBLIC FLAG_USE_UBUNTU=1)
endif()

if(NOEXECINFOH)
    target_compile_definitions(test_cloudstreamer PUBLIC NOEXECINFOH=1)
endif()

if(OLDCPP)
    target_compile_definitions(test_cloudstreamer PUBLIC OLDCPP=1)
endif()

if(WITH_OPENSSL)
    target_compile_definitions(test_cloudstreamer PUBLIC WITH_OPENSSL=1)
endif()


if (CMAKE_HOST_WIN32)
    target_compile_definitions(test_cloudstreamer PUBLIC WIN32=1)
    target_compile_definitions(test_cloudstreamer PUBLIC _WIN32=1)
    target_compile_definitions(test_cloudstreamer PUBLIC WIN32_LEAN_AND_MEAN=1)
    target_compile_definitions(test_cloudstreamer PUBLIC CURL_STATICLIB=1)

    target_link_libraries( # Specifies the target
                       test_cloudstreamer
                       ${cloudsdk}
                       ${jansson}
                       ${libwebsockets_static}
                       Ws2_32.lib
                       ${MSVCRTLIB}
                       Bcrypt.lib
                       wldap32.lib
                       Secur32.lib
                       advapi32.lib
                       wsock32.lib
                       kernel32.lib
                       user32.lib
                       gdi32.lib
                       winspool.lib
                       uuid.lib
                       )
else()

    target_link_libraries( # Specifies the target
                       test_cloudstreamer
                       # Links libraries to the target library.
		       pthread
                       ${cloudsdk}
                       ${jansson}
                       ${libwebsockets}
		       pthread
		       dl
			)

    if (FLAG_USE_UBUNTU)
    target_link_libraries( # Specifies the target
                       test_cloudstreamer
                       # Links libraries to the target library.
		       z
		       lzma
		       cap
			)

   endif()
endif()

if(WITH_OPENSSL)
    target_link_libraries( # Specifies the target
                       test_cloudstreamer
                       # Links libraries to the target library.
                       ${libssl}
                       ${libcrypto}
			)

endif()

