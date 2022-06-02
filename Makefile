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
# Generating a skeleton stage directory
#

stage-subdirs := bin include lib lib/jamfiles
all-stage-dirs := $(stage-dir) $(addprefix $(stage-dir)/,$(stage-subdirs))

.PHONY: skeleton-stage-dir
skeleton-stage-dir: $(stage-dir)/lib/jamfiles/Jamroot | $(all-stage-dirs)

$(all-stage-dirs):
	$(mkdir) "$(call to-shell,$@)"

define staged-jamroot-content :=
project staged-jamfiles ;

endef

$(stage-dir)/lib/jamfiles/Jamroot: | $(stage-dir)/lib/jamfiles
	$(file >$@,$(staged-jamroot-content))
	@echo generated $@

#
# Determine the bjam-specific build directory
#
bjam-build-dir := $(build-dir)/bjam

#
# $(call project-work-dir,<project>)
# Returns the (build setting-specific) working directory for <project>
#
project-work-dir = $(build-mode-dir)/work/$1

#
# $(call expand,<text>)
#
expand = $(if $(expand-info),$(info $1))$(eval $1)

#
# $(call define-gmake-project,<name> <version>?,<makefile>,<prereq name>*)
#
define gmake-project-definition =
#
# $1
#
$1.version := $2

.PHONY: $1
$1: $(addsuffix .stage,$4)
	$(MAKE) -C $(dir $3) -f $(notdir $3) -I "$(abspath include)" $(build-settings) work-dir="$(call project-work-dir,$1)" stage-dir="$(stage-dir)"

.PHONY: $1.stage
$1.stage: skeleton-stage-dir $(addsuffix .stage,$4)
	$(MAKE) -C $(dir $3) -f $(notdir $3) -I "$(abspath include)" $(build-settings) work-dir="$(call project-work-dir,$1)" stage-dir="$(stage-dir)" stage

.PHONY: $1.dist
$1.dist: $(addsuffix .stage,$4)
	$(MAKE) -C $(dir $3) -f $(notdir $3) -I "$(abspath include)" $(build-settings) work-dir="$(call project-work-dir,$1)" stage-dir="$(stage-dir)" dist

.PHONY: $1.clean
$1.clean:
	$(rmdir) "$(call to-shell,$(call project-work-dir,$1))"

endef

define-gmake-project = $(call expand,$(call gmake-project-definition,$(word 1,$1),$(word 2,$1),$2,$3))

#
# Determine bjam options
#
bjam-options := $(strip \
 $(if $(verbose),-d+2) \
 --build-dir="$(call to-native,$(bjam-build-dir))" \
 -sstage-dir="$(call to-native,$(stage-dir))" \
)

#
# Determine additional bjam options for .dist targets
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
# $(call define-bjam-project,<name> <version>?,<source dir>,<prereq name>*)
#
define bjam-project-definition =
#
# $1
#
$1.version := $2

.PHONY: $1
$1: $(addsuffix .stage,$4)
	$(bjam) $(bjam-options) $(build-settings) $3

# Legacy bjam project: no staging; consumers refer to source dir
.PHONY: $1.stage
$1.stage: $1

.PHONY: $1.dist
$1.dist: $(addsuffix .stage,$4)
	$(bjam) $$(bjam-dist-options) $(bjam-options) $(build-settings) $3//dist
	
.PHONY: $1.clean
$1.clean:
	$(bjam) --clean $(bjam-options) $(build-settings) $3

endef

define-bjam-project = $(call expand,$(call bjam-project-definition,$(word 1,$1),$(word 2,$1),$2,$3))

#
# Generated project targets
#
$(call define-bjam-project,cuti 0_0_0,cuti/cuti)
$(call define-bjam-project,cuti_unit_tests,cuti/unit_tests,cuti)

$(call define-bjam-project,x264_proto 0_0_0,x264_proto/x264_proto,cuti)

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
dist: x264_encoding_service.dist

PHONY: clean
clean:
	$(rmdir) "$(call to-shell,$(build-dir))"
