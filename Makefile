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
PATH := $(if $(windows),$(stage-dir)/lib:)$(PATH)
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
# $1 (gmake project)
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
# $(call gmake-project,<properties>)
#
# properties are:
# name      project name
# version?  version number
# makefile  path to makefile
# prereqs*  prerequisite projects
#
gmake-keys := name version makefile prereqs
gmake-project = $(call expand,$(call call-stripped, \
  gmake-project-impl, \
  $(call get-value,name,$1,$(gmake-keys)), \
  $(call find-value,version,$1,$(gmake-keys)), \
  $(call get-value,makefile,$1,$(gmake-keys)), \
  $(call find-values,prereqs,$1,$(gmake-keys)) \
))

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
# $1 (bjam legacy project)
#
$1.version := $2
$1.prereqs := $4

.PHONY: $1
$1: $1.all

.PHONY: $1.all
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
# $(call bjam-legacy-project,<properties>)
#
# properties are:
# name       project name
# version?   version number
# source-dir source directory
# prereqs*   prerequisite projects
#
bjam-legacy-keys := name version source-dir prereqs
bjam-legacy-project = $(call expand,$(call call-stripped, \
  bjam-legacy-project-impl, \
  $(call get-value,name,$1,$(bjam-legacy-keys)), \
  $(call find-value,version,$1,$(bjam-legacy-keys)), \
  $(call get-value,source-dir,$1,$(bjam-legacy-keys)), \
  $(call find-values,prereqs,$1,$(bjam-legacy-keys)) \
))

#
# $(call bjam-dll-project-impl,<name>,<version>,<source dir>,<include dir>,<prereq name>*)
#
define bjam-dll-project-impl =
#
# $1 (bjam dll project)
#
$1.version := $2
$1.prereqs := $5

.PHONY: $1
$1: $1.all

.PHONY: $1.all
$1.all: build-dir-skeleton $(addsuffix .stage,$5)
	$(bjam) $$(call bjam-options,$1) $(build-settings) $3

# We deliberately abuse recursive make here to avoid touching the
# jamfile in the stage dir when bjam-stage did nothing.  (The .dist
# targets depend on the .stage targets of their prerequisites, and
# .dist may be run as root).
.PHONY: $1.stage
$1.stage: $1.bjam-stage
	$$(MAKE) expand-info= build-dir=$(build-dir) $(build-settings) $(stage-dir)/lib/jamfiles/$1/jamfile

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
	$$(file >$$@,$$(call staged-jamfile-content,$1,$(stage-dir)/lib/$(call bjam-implib-filename,$1,$2),$(abspath $4),$5))
	@echo generated $$@

$(stage-dir)/lib/jamfiles/$1:
	$(mkdir) "$$(call to-shell,$$@)"
	
endef

#
# $(call bjam-dll-project,<properties>)
#
# properties are:
# name        project name
# version     version number
# source-dir  source directory
# include-dir include directory
# prereqs*    prerequisite projects
#
bjam-dll-keys := name version source-dir include-dir prereqs
bjam-dll-project = $(call expand,$(call call-stripped, \
  bjam-dll-project-impl, \
  $(call get-value,name,$1,$(bjam-dll-keys)), \
  $(call get-value,version,$1,$(bjam-dll-keys)), \
  $(call get-value,source-dir,$1,$(bjam-dll-keys)), \
  $(call get-value,include-dir,$1,$(bjam-dll-keys)), \
  $(call find-values,prereqs,$1,$(bjam-dll-keys)) \
))

#
# $(call bjam-statlib-project-impl,<name>,<source dir>,<include dir>,<prereq name>*)
#
define bjam-statlib-project-impl =
#
# $1 (bjam static library project)
#
$1.version :=
$1.prereqs := $4

.PHONY: $1
$1: $1.all

.PHONY: $1.all
$1.all: build-dir-skeleton $(addsuffix .stage,$4)
	$(bjam) $$(call bjam-options,$1) $(build-settings) $2

# We deliberately abuse recursive make here to avoid touching the
# jamfile in the stage dir when bjam-stage did nothing.  (The .dist
# targets depend on the .stage targets of their prerequisites, and
# .dist may be run as root).
.PHONY: $1.stage
$1.stage: $1.bjam-stage
	$$(MAKE) expand-info= build-dir=$(build-dir) $(build-settings) $(stage-dir)/lib/jamfiles/$1/jamfile

# Static library: nothing to do for disting
.PHONY: $1.dist
$1.dist: $1.all $(addsuffix .dist,$4)
	
.PHONY: $1.clean
$1.clean:
	$(bjam) --clean $$(call bjam-options,$1) $(build-settings) $2

.PHONY: $1.bjam-stage
$1.bjam-stage: $1.all
	$(bjam) $$(call bjam-options,$1) $(build-settings) $2//stage
	
$(stage-dir)/lib/jamfiles/$1/jamfile: $(stage-dir)/lib/$(call bjam-statlib-filename,$1) | $(stage-dir)/lib/jamfiles/$1
	$$(file >$$@,$$(call staged-jamfile-content,$1,$(stage-dir)/lib/$(call bjam-statlib-filename,$1),$(abspath $3),$4))
	@echo generated $$@

$(stage-dir)/lib/jamfiles/$1:
	$(mkdir) "$$(call to-shell,$$@)"
	
endef

#
# $(call bjam-statlib-project,<properties>)
#
# properties are:
# name        project name
# source-dir  source directory
# include-dir include directory
# prereqs*    prerequisite projects
#
bjam-statlib-keys := name source-dir include-dir prereqs
bjam-statlib-project = $(call expand,$(call call-stripped, \
  bjam-statlib-project-impl, \
  $(call get-value,name,$1,$(bjam-statlib-keys)), \
  $(call get-value,source-dir,$1,$(bjam-statlib-keys)), \
  $(call get-value,include-dir,$1,$(bjam-statlib-keys)), \
  $(call find-values,prereqs,$1,$(bjam-statlib-keys)) \
))

#
# $(call bjam-exe-project-impl,<name>,<source dir>,,<prereq name>*)
#
define bjam-exe-project-impl =
#
# $1 (bjam exe project)
#
$1.version := 
$1.prereqs := $3

.PHONY: $1
$1: $1.all

.PHONY: $1.all
$1.all: build-dir-skeleton $(addsuffix .stage,$3)
	$(bjam) $$(call bjam-options,$1) $(build-settings) $2

.PHONY: $1.dist
$1.dist: $1.all $(addsuffix .dist,$3)
	$(bjam) $$(call bjam-dist-options,$1) $(build-settings) $2//dist
	
.PHONY: $1.clean
$1.clean:
	$(bjam) --clean $$(call bjam-options,$1) $(build-settings) $2
	
endef

#
# $(call bjam-exe-project,<name>,<source dir>,<prereq name>*)
#
bjam-exe-project = $(call expand,$(call bjam-exe-project-impl,$1,$2,$3))

#
# $(call bjam-test-project-impl,<name>,<source dir>,<prereq name>*)
#
define bjam-test-project-impl =
#
# $1 (bjam test project)
#
$1.version :=
$1.prereqs := $3

.PHONY: $1
$1: $1.all

$1.all: build-dir-skeleton $(addsuffix .stage,$3)
	$(bjam) $$(call bjam-options,$1) $(build-settings) $2

.PHONY: $1.clean
$1.clean:
	$(bjam) --clean $$(call bjam-options,$1) $(build-settings) $2

# Pattern rule for running a specific test
.PHONY: $1.%
$1.%: build-dir-skeleton $(addsuffix .stage,$3)
	$(bjam) $$(call bjam-options,$1) $(build-settings) $2//$$*

endef

#
# $(call bjam-test-project,<name>,<source dir>,<prereq name>*)
#
bjam-test-project = $(call expand,$(call bjam-test-project-impl,$1,$2,$3))

#
# Generated project targets
#
$(call bjam-exe-project,x264_encoding_service,x264_encoding_service,x264_es_utils cuti)

$(call bjam-statlib-project, \
  name: x264_es_utils \
  source-dir: x264_es_utils/x264_es_utils \
  include-dir: x264_es_utils \
  prereqs: x264_proto cuti x264 \
)

$(call bjam-test-project,x264_es_utils_unit_tests,x264_es_utils/unit_tests,x264_es_utils)

$(call bjam-dll-project, \
  name: x264_proto \
  version: 0_0_0 \
  source-dir: x264_proto/x264_proto \
  include-dir: x264_proto \
  prereqs: cuti \
)

$(call bjam-dll-project, \
  name: cuti \
  version: 0_0_0 \
  source-dir: cuti/cuti \
  include-dir: cuti \
)

$(call bjam-test-project,cuti_unit_tests,cuti/unit_tests,cuti)

$(call gmake-project, \
  name: x264 \
  makefile: x264/USPMakefile \
)

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
