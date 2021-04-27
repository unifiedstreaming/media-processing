#!/bin/sh -ex

b2 -d+2 variant=debug
b2 -d+2 variant=debug unit_tests

b2 -d+2 variant=release
b2 -d+2 variant=release unit_tests

if test ! -r /etc/os-release || ! grep -iq alpine /etc/os-release; then
  # Sanitizers do not work on Alpine Linux.

  b2 -d+2 variant=debug address-sanitizer=on
  b2 -d+2 variant=debug address-sanitizer=on unit_tests

  b2 -d+2 variant=release address-sanitizer=on
  b2 -d+2 variant=release address-sanitizer=on unit_tests

  b2 -d+2 variant=debug thread-sanitizer=on
  b2 -d+2 variant=debug thread-sanitizer=on unit_tests

  b2 -d+2 variant=release thread-sanitizer=on
  b2 -d+2 variant=release thread-sanitizer=on unit_tests
fi
