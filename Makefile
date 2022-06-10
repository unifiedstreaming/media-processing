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
# Recursive Make Considered Harmful: prevent concurrent captains on
# the ship (bjam and/or GNUmake) each assuming they're the only
# process generating build artifacts in (potentially) shared locations
# such as the bjam build directory (over which we have no control),
# the staging directory or make dist's $(dest-dir).
#
# Please note that sub-makes will still run parallel jobs if -j<X> is
# passed to this Makefile, and that bjam defaults to some level of
# parallel builds by default.
#
.NOTPARALLEL:

#
# Determine the build directory
#
override build-dir := $(abspath $(if $(build-dir),$(call to-make,$(build-dir)),build))

#
# Determine the bjam-specific build directory
#
bjam-build-dir := $(build-dir)/bjam

#
# Determine the (build setting-specific) directory for generated artifacts 
#
build-mode-dir := $(build-dir)/other/$(subst $(space),/,$(subst =,-,$(build-settings)))

#
# Determine the (build setting-specific) staging directory
#
stage-dir := $(build-mode-dir)/stage

#
# Generating a skeleton build directory
#

skeleton-build-dirs := \
  $(build-dir) \
  $(bjam-build-dir) \
  $(build-mode-dir) \
  $(stage-dir) \
  $(stage-dir)/include \
  $(stage-dir)/lib \
  $(stage-dir)/lib/jamfiles
  
.PHONY: build-dir-skeleton
build-dir-skeleton: $(stage-dir)/lib/jamfiles/Jamroot | $(skeleton-build-dirs)

$(bjam-build-dir): | $(build-dir)
$(build-mode-dir): | $(build-dir)
$(stage-dir): | $(build-mode-dir)
$(stage-dir)/include: | $(stage-dir)
$(stage-dir)/lib: | $(stage-dir)
$(stage-dir)/lib/jamfiles: | $(stage-dir)/lib

$(skeleton-build-dirs):
	$(mkdir) "$(call to-shell,$@)"

define staged-jamroot-content :=
project staged-jamfiles ;

endef

$(stage-dir)/lib/jamfiles/Jamroot: | $(stage-dir)/lib/jamfiles
	$(file >$@,$(staged-jamroot-content))
	@echo generated $@

#
# Windows: unit tests must find DLLs in $(stage-dir)/lib
#
PATH := $(if $(windows),$(stage-dir)/lib:)$(PATH))
export PATH

#
# $(call project-work-dir,<project>)
# Returns the (build setting-specific) working directory for <project>
#
project-work-dir = $(build-mode-dir)/work/$1

#
# $(call recursive-prereqs,<project name>)
#
recursive-prereqs-impl = $(foreach prereq,$($1.prereqs), \
  $(prereq) $(call recursive-prereqs-impl,$(prereq)) \
)
recursive-prereqs = $(call keep-last-instance,$(call recursive-prereqs-impl,$1))

#
# $(call required-versions,<project name>)
#
required-versions = $(strip \
  $(foreach proj,$1 $(call recursive-prereqs,$1), \
    $(if $($(proj).version),$(proj).version=$($(proj).version)) \
  ) \
)

#
# $(call expand,<text>)
#
expand = $(if $(expand-info),$(info $1))$(eval $1)

#
# $(call gmake-options,<project name>)
# (to be evaluated very lazily, as part of a recipe)
#
gmake-options = $(strip \
  -I "$(abspath include)" \
  work-dir="$(call project-work-dir,$1)" \
  stage-dir="$(stage-dir)" \
  $(call required-versions,$1) \
)
  
#
# $(call gmake-dist-options,<project name>)
# (to be evaluated very lazily, as part of a recipe)
#
gmake-dist-options = $(strip \
  $(call gmake-options,$1) \
  dest-dir="$(call to-shell,$(call required-value,dest-dir))" \
)

#
# $(call gmake-project-impl,<name>,<version>?,<makefile>,<prereq name>*)
#
define gmake-project-impl =
#
# $1
#
$1.version := $2
$1.prereqs := $4

.PHONY: $1
$1: $1.all

.PHONY: $1.all
$1.all: build-dir-skeleton $(addsuffix .stage,$4)
	$$(MAKE) -C $(dir $3) -f $(notdir $3) $$(call gmake-options,$1) $(build-settings) all

.PHONY: $1.stage
$1.stage: $1.all
	$$(MAKE) -C $(dir $3) -f $(notdir $3) $$(call gmake-options,$1) $(build-settings) stage

.PHONY: $1.dist
$1.dist: $1.all $(addsuffix .dist,$4)
	$$(MAKE) -C $(dir $3) -f $(notdir $3) $$(call gmake-dist-options,$1) $(build-settings) dist

.PHONY: $1.clean
$1.clean:
	$(rmdir) "$(call to-shell,$(call project-work-dir,$1))"

endef

#
# $(call gmake-project,<name> <version>?,<makefile>,<prereq name>*)
#
gmake-project = $(call expand,$(call gmake-project-impl,$(word 1,$1),$(word 2,$1),$2,$3))

#
# $(call bjam-options,<project name>)
# (to be evaluated very lazily, as part of a recipe)
#
bjam-options = $(strip \
 $(if $(verbose),-d+2) \
 --build-dir="$(call to-native,$(bjam-build-dir))" \
 -sstage-dir="$(call to-native,$(stage-dir))" \
 $(addprefix -s,$(call required-versions,$1)) \
)

#
# $(call bjam-dist-options,<project name>)
# (to be evaluated very lazily, as part of a recipe)
#
bjam-dist-options = $(strip \
  $(call bjam-options,$1) \
  -sdest-dir="$(call to-native,$(call required-value,dest-dir))" \
)

#
# $(call bjam-legacy-project-impl,<name>,<version>?,<source dir>,<prereq name>*)
#
define bjam-legacy-project-impl =
#
# $1
#
$1.version := $2
$1.prereqs := $4

.PHONY: $1
$1: $1.all

$1.all: build-dir-skeleton $(addsuffix .stage,$4)
	$(bjam) $$(call bjam-options,$1) $(build-settings) $3

# Legacy bjam project: no staging; consumers can only be other
# jamfiles referring to this project's jamfile
.PHONY: $1.stage
$1.stage: $1.all

.PHONY: $1.dist
$1.dist: $1.all $(addsuffix .dist,$4)
	$(bjam) $$(call bjam-dist-options,$1) $(build-settings) $3//dist
	
.PHONY: $1.clean
$1.clean:
	$(bjam) --clean $$(call bjam-options,$1) $(build-settings) $3

endef

#
# $(call bjam-legacy-project,<name> <version>?,<source dir>,<prereq name>*)
#
bjam-legacy-project = $(call expand,$(call bjam-legacy-project-impl,$(word 1,$1),$(word 2,$1),$2,$3))

#
# $(call bjam-dll-project-impl,<name>,<version>,<source dir>,<header dir>,<prereq name>*)
#
define bjam-dll-project-impl =
#
# $1
#
$1.version := $(if $2,$2,$(error missing version for bjam-dll-project $1))
$1.prereqs := $5

.PHONY: $1
$1: $1.all

$1.all: build-dir-skeleton $(addsuffix .stage,$5)
	$(bjam) $$(call bjam-options,$1) $(build-settings) $3

# We deliberately abuse recursive make here to avoid touching the
# jamfile in the stage dir after bjam-stage did nothing.  (The .dist
# targets depend on .stage, and .dist may be run as root).
.PHONY: $1.stage
$1.stage: $1.bjam-stage
	$$(MAKE) build-dir=$(build-dir) $(build-settings) $(stage-dir)/lib/jamfiles/$1/jamfile

.PHONY: $1.dist
$1.dist: $1.all $(addsuffix .dist,$5)
	$(bjam) $$(call bjam-dist-options,$1) $(build-settings) $3//dist
	
.PHONY: $1.clean
$1.clean:
	$(bjam) --clean $$(call bjam-options,$1) $(build-settings) $3

.PHONY: $1.bjam-stage
$1.bjam-stage: $1.all
	$(bjam) $$(call bjam-options,$1) $(build-settings) $3//stage
	
$(stage-dir)/lib/jamfiles/$1/jamfile: $(stage-dir)/lib/$(call bjam-implib-filename,$1,$2) | $(stage-dir)/lib/jamfiles/$1
	$$(file >$$@,$$(call staged-jamfile-content,$1,$(stage-dir)/lib/$(call bjam-implib-filename,$1,$2),$(abspath $4)))
	@echo generated $$@

$(stage-dir)/lib/jamfiles/$1:
	$(mkdir) "$$(call to-shell,$$@)"
	
endef

#
# $(call bjam-dll-project,<name> <version>,<source dir>,<header dir>,<prereq name>*)
#
bjam-dll-project = $(call expand,$(call bjam-dll-project-impl,$(word 1,$1),$(word 2,$1),$2,$3,$4))

#
# Generated project targets
#
$(call bjam-legacy-project,x264_encoding_service,x264_encoding_service,x264_es_utils cuti)

$(call bjam-legacy-project,x264_es_utils,x264_es_utils/x264_es_utils,x264_proto cuti x264)
$(call bjam-legacy-project,x264_es_utils_unit_tests,x264_es_utils/unit_tests,x264_es_utils)

$(call bjam-legacy-project,x264_proto 0_0_0,x264_proto/x264_proto,cuti)

$(call bjam-dll-project,cuti 0_0_0,cuti/cuti,cuti)
$(call bjam-legacy-project,cuti_unit_tests,cuti/unit_tests,cuti)

$(call gmake-project,x264,x264/USPMakefile)

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
