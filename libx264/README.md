Unified Streaming fork of x264 README
=====================================

[x264](https://www.videolan.org/developers/x264.html) is a free software library
and application for encoding video streams into the H.264/MPEG-4 AVC compression
format, and is released under the terms of the GNU GPL.

This repository contains a fork of the x264 sources, with customizations used by
[Unified Streaming](https://www.unified-streaming.com/):

* Added shell scripts for creating GPLv2 builds of x264 with gcc/clang and msvc
* Added jamfile.v2 for building with [b2](https://www.bfgroup.xyz/b2/)
* Renamed upstream `version.sh` to `version.sh.orig`, and added `version.sh`
  which outputs version defines without requiring git.
