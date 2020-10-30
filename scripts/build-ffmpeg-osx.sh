#!/bin/bash

set -e

mkdir -p ../libs
pushd ../libs
mkdir -p ffmpeg.build
pushd ffmpeg.build

# ../ffmpeg/configure --enable-shared --disable-static --enable-gpl --enable-libx264 --install-name-dir='@executable_path/../Resources/ffmpeg_libs' --prefix=`pwd` --libdir=`pwd`/ffmpeg_libs
../$1/configure --enable-shared --disable-static --install-name-dir='@executable_path/../Resources/x86_64' --libdir=`pwd`/../MacOSX/x86_64 --prefix=`pwd` --incdir=`pwd`/../include
make -j 8
make install

popd
popd
