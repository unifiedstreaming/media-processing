# Unified Streaming media processing source repository

This repository contains the source code for Unified Streaming's open source
media processing tools.

## Repository contents

* **cuti**

  A shared library/DLL that provides an asynchronous RPC framework for
  C++, and a few other utilties for logging, command line parsing and
  daemonization. Licensed under the LGPLv2.1.

* **usp-builder**

  A build tool that is mainly used for managing the dependencies
  between the projects in the source tree, relying on
  [b2](https://www.bfgroup.xyz/b2/) and GNU make to do the actual
  work. Licensed under the GPLv2.

* **with_dll_dir**

  A small Python-on-Windows wrapper to set the native DLL directory
  search directory for a python script. Licensed under the LGPLv2.1.

* **x264**

  Our fork of VideoLAN's
  [x264](https://www.videolan.org/developers/x264.html)
  library. Licensed under the GPLv2.

* **x264_encoding_service**

  A daemon/service that provides x264 encoding over a TCP
  connection. Licensed under the GPLv2.

* **x264_es_utils**

  A static library providing some utilities for the x264 encoding
  service. Licensed under the GPLv2.

* **x264_proto**

  A shared library/DLL that implements both the client and server
  sides of the network protocol used by the x264 encoding
  service. Licensed under the LGPLv2.1.

## Build requirements

* A recent *native* C++ compiler toolchain. We use g++ on Linux, and
  the MSVC command line tools on Windows. Decent support for C++-20
  is required.

* A recent *native* version of [b2](https://www.bfgroup.xyz/b2/).

* A recent version of **GNU make** and a few other POSIX-style command
  line utilities such as **bash**, **rm**, **mkdir** and **GNU
  sed**. On Windows, we use a fairly minimal set of Cygwin packages to
  provide these tools; the resulting binaries do not depend on Cygwin.

## Build instructions

From a POSIX-style shell (bash) in the top level source directory:

* type `make` to build all executables and shared libs/DLLs. By
  default, debug mode is used. For release mode, type `make
  variant=release`

* type `make clean` to remove all generated artifacts.

* type `make unit_tests` to run all unit tests.

* type `make deploy dest-dir=/some/path` to deploy the executables
  and shared libs/DLLs to the directory tree rooted at `/some/path`.
