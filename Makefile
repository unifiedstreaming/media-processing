#
# "THE BEER-WARE LICENSE" (Revision CS-42):
#
# This file was written by the CodeShop developers.  As long as you
# retain this notice you can do whatever you want with it.
# If we meet some day, and you think this file is worth it, you can
# buy us a beer in return.  Even if you do that, this file still
# comes WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#

#
# Set GNU make options
#
.DELETE_ON_ERROR:

#
# Define utilities
#
override EMPTY :=
override SPACE := $(EMPTY) $(EMPTY)
override escape = $(subst $(SPACE),\$(SPACE),$1)
override unescape = $(subst \$(SPACE),$(SPACE),$1)

#
# Determine this file's directory
#
TOP := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))

#
# to_make converts a path passed to make's command line to a path
# understood by make (forward slashes as directory separators,
# spaces escaped with backslashes)
#
to_make = $(call escape,$(subst \,/,$1))

#
# Determine if we're on a windows system, the unix-likeness of that
# system, the name of the null device, how to obtain a path to use on
# the shell command line, and how to obtain a path passed to native
# executables.
#

ifeq ($(findstring Windows,$(OS)),)

  # Not on Windows; assume vanilla Unix
  WINDOWS :=
  UNIX_LIKE := yes
  DEV_NULL := /dev/null
  to_shell = $(call unescape,$1)
  to_native = $(call unescape,$1)

else ifneq ($(filter sh bash dash,$(basename $(notdir $(wildcard $(SHELL))))),)

  # Windows with familiar UNIX-like $(SHELL) file
  WINDOWS := yes
  UNIX_LIKE := yes
  DEV_NULL := $(if $(wildcard /dev/null),/dev/null,NUL)
  to_shell = $(call unescape,$1)

  ifneq ($(filter /bin/ /usr/bin/,$(dir $(shell which cygpath 2>$(DEV_NULL)))),)
    # Using a cygwinesque shell; path mapping required
    to_native = $(shell cygpath --windows $(call unescape,$1))
  else
    # Assume single filesystem view
    to_native = $(subst /,\,$(call unescape,$1))
  endif

else

  # Windows fallback when $(SHELL) file not found or unfamiliar shell name
  WINDOWS := yes
  UNIX_LIKE :=
  DEV_NULL := NUL
  to_shell = $(subst /,\,$(call unescape,$1))
  to_native = $(subst /,\,$(call unescape,$1))

endif

#
# Define utility commands for the shell
#
ifneq ($(UNIX_LIKE),)
  CP := cp
  MKDIR := mkdir -p
  RMDIR := rm -rf
else
  CP := copy /Y
  MKDIR := mkdir
  RMDIR := -rmdir /S /Q
endif

#
# Determine bjam-like build settings
#
define BUILD_SETTING_NAMES :=
  address-model
  address-sanitizer
  thread-sanitizer
  toolset
  undefined-sanitizer
  variant
endef

BUILD_SETTINGS := $(strip \
  $(foreach name,$(BUILD_SETTING_NAMES), \
    $(if $($(name)),$(name)=$($(name))) \
  ) \
)

#
# Determine the build directory
#
BUILD_DIR := $(call to_make,$(if $(build-dir),$(build-dir),obj))

#
# Determine the directory to install to
#
ifneq ($(filter install,$(MAKECMDGOALS)),)
  ifeq ($(prefix),)
    $(error target 'install' requires 'prefix=<somehere>')
  endif
endif
PREFIX := $(call to_make,$(prefix))

#
# Determine bjam args; we assume bjam is a native executable
#
BJAM_OPTIONS := --build-dir="$(call to_native,$(BUILD_DIR)/bjam)"
ifneq ($(PREFIX),)
  BJAM_OPTIONS += --prefix="$(call to_native,$(PREFIX))"
  ifneq ($(WINDOWS),)
    # Ensure dlls are placed in the directory for executables
    BJAM_OPTIONS += --libdir="$(call to_native,$(PREFIX)/bin)"
  endif
endif

ifneq ($(verbose),)
  BJAM_OPTIONS += -d+2
endif

bjam_args = $(BJAM_OPTIONS) $(BUILD_SETTINGS) $(patsubst %/,%,$(dir $1))$(addprefix //,$(basename $(notdir $1)))

#
# Determine the name of the bjam executable, preferring 'b2' over 'bjam'
#
BJAM := $(if $(shell b2 --version 2>$(DEV_NULL)),b2,bjam)

.PHONY: all
all : x264_encoding_service/.bjam

#
# (Pattern) rules for calling bjam
#
.PHONY : .bjam
.bjam: | $(BUILD_DIR)
	$(BJAM) $(call bjam_args,$@)

.PHONY : %.bjam
%.bjam: | $(BUILD_DIR)
	$(BJAM) $(call bjam_args,$@)

.PHONY : clean
clean :
	$(RMDIR) "$(call to_shell,$(BUILD_DIR))"

.PHONY : install
install : install.bjam

install.bjam : | $(PREFIX)

.PHONY : unit_tests
unit_tests : unit_tests.bjam

$(BUILD_DIR) :
	$(MKDIR) "$(call to_shell,$@)"

$(PREFIX) :
	$(MKDIR) "$(call to_shell,$@)"
