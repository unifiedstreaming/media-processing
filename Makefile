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

include include/USPCommon.mki

#
# Determine the build directory
#
override build-dir := $(abspath $(if $(build-dir),$(call to-make,$(build-dir)),build))

#
# Determine the (build setting-specific) directory for generated artifacts 
#
build-mode-dir := $(build-dir)/other/$(subst $(space),/,$(subst =,-,$(build-settings)))

#
# Determine the (build setting-specific) staging directory
#
stage-dir := $(build-mode-dir)/stage

#
# Determine the bjam-specific build directory
#
bjam-build-dir := $(build-dir)/bjam

#
# $(call gmake-work-dir,<project>)
# Returns the (build setting-specific) working directory for <project>
#
gmake-work-dir = $(build-mode-dir)/work/$1

#
# Generating $(stage-dir)/lib/jamfiles/Jamroot
#
define staged-jamroot-content :=
project staged-jamfiles ;

endef

$(stage-dir)/lib/jamfiles/Jamroot: | $(stage-dir)/lib/jamfiles
	$(file >$@,$(staged-jamroot-content))
	@echo generated $@

$(stage-dir)/lib/jamfiles:
	$(mkdir) "$(call to-shell,$@)"

#
# $(call expand,<text>)
#
expand = $(if $(expand-info),$(info $1))$(eval $1)

#
# $(call define-gmake-project,<name>,<makefile>,<prereq project name>*)
#
define gmake-project-definition =
.PHONY: $1
$1: $(stage-dir)/lib/jamfiles/Jamroot $3
	$(MAKE) -C $(dir $2) -f $(notdir $2) -I "$(abspath include)" $(build-settings) work-dir="$(call gmake-work-dir,$1)" stage-dir="$(stage-dir)" stage

.PHONY: clean-$1
clean-$1:
	$(rmdir) "$(call to-shell,$(call gmake-work-dir,$1))"

endef

define-gmake-project = $(call expand,$(call gmake-project-definition,$1,$2,$3))

#
# Determine bjam options & args
#
bjam-options := $(strip \
 $(if $(verbose),-d+2) \
 --build-dir="$(call to-native,$(bjam-build-dir))" \
 -sstage-dir="$(call to-native,$(stage-dir))" \
)

#
# Determine bjam dist options
#
# Lazily evaluated from some build recipes, so the requirements
# on $(dest-dir) only kick in when that build recipe is run
#
bjam-dist-options = $(strip \
  --prefix="$(call to-native,$(call required-value,dest-dir))" \
  $(if $(windows), \
    --libdir="$(call to-native,$(call required-value,dest-dir)/bin)" \
  ) \
)

#
# $(call define-bjam-project,<name>,<source dir>,<prereq project name>*)
#
define bjam-project-definition =
.PHONY: $1
$1: $3
	$(bjam) $(bjam-options) $(build-settings) $2

.PHONY: dist-$1
dist-$1: $3
	$(bjam) $$(bjam-dist-options) $(bjam-options) $(build-settings) $2//dist
	
.PHONY: clean-$1
clean-$1:
	$(bjam) --clean $(bjam-options) $(build-settings) $2

endef

define-bjam-project = $(call expand,$(call bjam-project-definition,$1,$2,$3))

#
# Generated project targets
#
$(call define-bjam-project,cuti,cuti/cuti)
$(call define-bjam-project,cuti_unit_tests,cuti/unit_tests,cuti)

$(call define-bjam-project,x264_proto,x264_proto/x264_proto,cuti)

$(call define-gmake-project,x264,x264/USPMakefile)

$(call define-bjam-project,x264_es_utils,x264_es_utils/x264_es_utils,x264_proto cuti x264)
$(call define-bjam-project,x264_es_utils_unit_tests,x264_es_utils/unit_tests,x264_es_utils)

$(call define-bjam-project,x264_encoding_service,x264_encoding_service,x264_es_utils cuti)

#
# User-level targets
#
.PHONY: all
all: x264_encoding_service

.PHONY: unit_tests
unit_tests: cuti_unit_tests x264_es_utils_unit_tests

.PHONY: dist
dist: dist-x264_encoding_service

PHONY: clean
clean:
	$(rmdir) "$(call to-shell,$(build-dir))"
