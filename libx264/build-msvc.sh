#!/bin/sh -eux

# "THE BEER-WARE LICENSE" (Revision CS-42):
#
# This file was written by the CodeShop developers.  As long as you
# retain this notice you can do whatever you want with it.
# If we meet some day, and you think this file is worth it, you can
# buy us a beer in return.  Even if you do that, this file still
# comes WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

# Compiler: MSVC 14 (aka Visual Studio 2019)
# Resulting license: GPL v2

# Debug build
build_dir=bld64-msvc-14/debug
install_dir=../../out64-msvc-14/debug
nproc=${NUMBER_OF_PROCESSORS}

rm -rf ${build_dir}
mkdir -p ${build_dir}
cd ${build_dir}

CC=cl.exe                       \
../../configure                 \
                                \
  --prefix=${install_dir}       \
                                \
  --extra-cflags="-MDd"         \
                                \
  --enable-static               \
  --disable-opencl              \
                                \
  --enable-debug                \
  --enable-pic                  \
                                \
  --disable-avs                 \
  --disable-swscale             \
  --disable-lavf                \
  --disable-ffms                \
  --disable-gpac                \
  --disable-lsmash

make -j${nproc} V=1

make V=1 install

cd ../../

# Release build
build_dir=bld64-msvc-14/release
install_dir=../../out64-msvc-14/release
nproc=${NUMBER_OF_PROCESSORS}

rm -rf ${build_dir}
mkdir -p ${build_dir}
cd ${build_dir}

CC=cl.exe                       \
../../configure                 \
                                \
  --prefix=${install_dir}       \
                                \
  --extra-cflags="-MD"          \
                                \
  --enable-static               \
  --disable-opencl              \
                                \
  --enable-pic                  \
                                \
  --disable-avs                 \
  --disable-swscale             \
  --disable-lavf                \
  --disable-ffms                \
  --disable-gpac                \
  --disable-lsmash

make -j${nproc} V=1

make V=1 install

cd ../../

# Sanity check include dirs: debug and release should be exactly identical.
diff -qr out64-msvc-14/debug/include out64-msvc-14/release/include

# We need only one copy of the headers.
rm -rf out64-msvc-14/include out64-msvc-14/debug/include
mv out64-msvc-14/release/include out64-msvc-14

# Move executables and libraries from out64 subdirs.
mv \
  out64-msvc-14/debug/bin/*.exe \
  out64-msvc-14/debug/lib/*.lib \
  out64-msvc-14/debug

mv \
  out64-msvc-14/release/bin/*.exe \
  out64-msvc-14/release/lib/*.lib \
  out64-msvc-14/release

# Cleanup out64 subdir leftovers that we don't want.
rm -rf \
  out64-msvc-14/debug/bin \
  out64-msvc-14/debug/lib \
  out64-msvc-14/release/bin \
  out64-msvc-14/release/lib \

# Copy pdb file from bld64 subdir.
cp \
  bld64-msvc-14/debug/*.pdb \
  out64-msvc-14/debug

# Show end result.
ls -l \
  out64-msvc-14/include \
  out64-msvc-14/debug \
  out64-msvc-14/release
