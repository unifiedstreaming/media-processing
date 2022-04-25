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
# Determine this file's directory
#
TOP := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))

include include/USPCommon.mki

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
BUILD_DIR := $(call to-make,$(if $(build-dir),$(build-dir),obj))

#
# Determine the directory to install to
#
ifneq ($(filter install,$(MAKECMDGOALS)),)
  ifeq ($(prefix),)
    $(error target 'install' requires 'prefix=<somehere>')
  endif
endif
PREFIX := $(call to-make,$(prefix))

#
# Determine bjam args; we assume bjam is a native executable
#
BJAM_OPTIONS := --build-dir="$(call to-native,$(BUILD_DIR)/bjam)"
ifneq ($(PREFIX),)
  BJAM_OPTIONS += --prefix="$(call to-native,$(PREFIX))"
  ifneq ($(windows),)
    # Ensure dlls are placed in the directory for executables
    BJAM_OPTIONS += --libdir="$(call to-native,$(PREFIX)/bin)"
  endif
endif

ifneq ($(verbose),)
  BJAM_OPTIONS += -d+2
endif

bjam_args = $(BJAM_OPTIONS) $(BUILD_SETTINGS) $(patsubst %/,%,$(dir $1))$(addprefix //,$(basename $(notdir $1)))

#
# Determine the name of the bjam executable, preferring 'b2' over 'bjam'
#
BJAM := $(if $(shell b2 --version 2>$(dev-null)),b2,bjam)

#
# Supported user targets
# 
all : .phony x264_encoding_service/.bjam

install : .phony install.bjam

unit_tests : .phony unit_tests.bjam

clean : .phony
	$(rmdir) "$(call to-shell,$(BUILD_DIR))"

.PHONY : .phony
.phony : 

#
# Rules for calling bjam
#
.bjam: .phony | $(BUILD_DIR)
	$(BJAM) $(call bjam_args,$@)

%.bjam: .phony | $(BUILD_DIR)
	$(BJAM) $(call bjam_args,$@)

install.bjam : | $(PREFIX)

#
# Directory creators
#
$(BUILD_DIR) :
	$(mkdir) "$(call to-shell,$@)"

$(PREFIX) :
	$(mkdir) "$(call to-shell,$@)"
