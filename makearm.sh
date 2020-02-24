#!/bin/sh
cmake -DCMAKE_BUILD_TYPE=Release -DTARGET_TYPE=ARM -DTOOLCHAIN_PATH=../../SDK-481/ -DTOOLCHAIN_PREFIX=arm-obreey-linux-gnueabi -DTOOLCHAIN_INSTALL=sysroot/usr -DPLATFORM=ARM
make

