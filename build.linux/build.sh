#!/bin/bash

HOME=`pwd`

if [ "$1" = "clean" ]; then
 rm -f -r build_jansson/*
 rm -f -r build_websockets/*
 rm -f -r build_cloudsdk/*
 rm -f -r build_test/*
fi

cd $HOME
echo "=>build jansson lib"
if [ ! -d "build_jansson" ]; then
    mkdir build_jansson
fi
cd build_jansson
if [ ! -f "Makefile" ]; then
    cmake ../../external_libs/jansson-2.11
fi
make
echo "<=build jansson lib"

cd $HOME
echo "=>build libwebsockets lib"
if [ ! -d "build_libwebsockets" ]; then
    mkdir build_libwebsockets
fi
cd build_libwebsockets
if [ ! -f "Makefile" ]; then
    cmake -DCMAKE_C_FLAGS="-fPIC" -DLWS_WITH_SSL=0 -DLWS_WITHOUT_TESTAPPS=1 -DLWS_WITHOUT_SERVER=1 -DLWS_WITHOUT_TEST_SERVER=1 -DLWS_WITHOUT_TEST_SERVER_EXTPOLL=1 -DLWS_WITHOUT_TEST_PING=1 -DLWS_WITHOUT_TEST_CLIENT=1 ../../external_libs/libwebsockets/src
fi
make
echo "<=build libwebsockets lib"

cd $HOME
echo "=>build cloudsdk lib"
if [ ! -d "build_cloudsdk" ]; then
    mkdir build_cloudsdk
fi
cd build_cloudsdk
if [ ! -f "Makefile" ]; then
    cmake -DJANSSON_BUILD_DIR=$HOME/build_jansson -DLIBWEBSOCKETS_BUILD_DIR=$HOME/build_libwebsockets ../../src
fi
make
echo "<=build cloudsdk lib"

cd $HOME
echo "=>build test_cloudstreamer app"
if [ ! -d "build_test" ]; then
    mkdir build_test
fi
cd build_test
if [ ! -f "Makefile" ]; then
    cmake -DFLAG_USE_UBUNTU=1 -DJANSSON_BUILD_DIR=$HOME/build_jansson -DLIBWEBSOCKETS_BUILD_DIR=$HOME/build_libwebsockets -DCLOUDSDK_BUILD_DIR=$HOME/build_cloudsdk ../../test
fi
make
echo "<=build test_cloudstreamer app"
