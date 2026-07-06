#!/bin/sh
# Script modified from upstream source, since we do not have the full x264 git
# history in our repository.
echo '#define X264_REV 3223'
echo '#define X264_REV_DIFF 0'
echo '#define X264_VERSION " r3223 0480cb0"'
echo '#define X264_POINTVER "0.165.3223 0480cb0"'
