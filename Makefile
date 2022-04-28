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
top := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))

include include/USPCommon.mki

#
# Determine bjam-like build settings
#
define build-setting-names :=
  address-model
  address-sanitizer
  thread-sanitizer
  toolset
  undefined-sanitizer
  variant
endef

build-settings := $(strip \
  $(foreach name,$(build-setting-names), \
    $(if $($(name)),$(name)=$($(name))) \
  ) \
)

#
# Determine the build directory
#
build-dir := $(call to-make,$(if $(build-dir),$(build-dir),obj))

#
# Determine the prerequistes directory bjam targets can refer to
#
prereqs-dir := $(abspath $(build-dir)/prereqs/$(subst $(space),/,$(subst =,-,$(build-settings))))
prereqs-install-dir := $(prereqs-dir)/install
prereqs-build-dir := $(prereqs-dir)/build

#
# Determine the directory to install to
#
ifneq ($(filter install,$(MAKECMDGOALS)),)
  ifeq ($(install-dir),)
    $(error target 'install' requires 'install-dir=<somehere>')
  endif
endif
install-dir := $(call to-make,$(install-dir))

#
# Determine bjam args; we assume bjam is a native executable
#
bjam-options := -sprereqs-install-dir="$(call to-native,$(prereqs-install-dir))"
bjam-options += --build-dir="$(call to-native,$(build-dir)/bjam)"
ifneq ($(install-dir),)
  bjam-options += --prefix="$(call to-native,$(install-dir))"
  ifneq ($(windows),)
    # Ensure dlls are placed in the directory for executables
    bjam-options += --libdir="$(call to-native,$(install-dir)/bin)"
  endif
endif

ifneq ($(verbose),)
  bjam-options += -d+2
endif

#
# $(call bjam_args, <bjam target>)
#
bjam_args = $(bjam-options) $(build-settings) $(patsubst %/,%,$(dir $1))$(addprefix //,$(basename $(notdir $1)))

#
# Determine the name of the bjam executable, preferring 'b2' over 'bjam'
#
bjam := $(if $(shell b2 --version 2>$(dev-null)),b2,bjam)

#
# Supported user targets
# 
all : .phony x264_encoding_service/.bjam

install : .phony install.bjam

unit_tests : .phony unit_tests.bjam

clean : .phony
	$(rmdir) "$(call to-shell,$(build-dir))"

.PHONY : .phony
.phony : 

#
# Rules for calling bjam
#
.bjam: .phony libx264 | $(build-dir)
	$(bjam) $(call bjam_args,$@)

%.bjam: .phony libx264 | $(build-dir)
	$(bjam) $(call bjam_args,$@)

install.bjam : | $(install-dir)

#
# Prequisite libraries
#
libx264 : .phony
	$(MAKE) -C x264 -f USPMakefile $(build-settings) build-dir=$(prereqs-build-dir)/x264 install-dir=$(prereqs-install-dir) install

#
# Directory creators
#
$(build-dir) :
	$(mkdir) "$(call to-shell,$@)"

$(install-dir) :
	$(mkdir) "$(call to-shell,$@)"
