#
# Copyright (C) 2022-2023 CodeShop B.V.
#
# This file is part of the usp-builder project.
#
# The usp-builder project is free software: you can redistribute it
# and/or modify it under the terms of version 2 of the GNU General
# Public License as published by the Free Software Foundation.
#
# The usp-builder project is distributed in the hope that it will be
# useful, but WITHOUT ANY WARRANTY; without even the implied warranty
# of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See version
# 2 of the GNU General Public License for more details.
#
# You should have received a copy of version 2 of the GNU General
# Public License along with the usp-builder project.  If not, see
# <http://www.gnu.org/licenses/>.
#

include usp-builder/USPCommon.mki

#
# $(call soname,<path>)
#
soname = $(shell \
  objdump -p "$1" 2>/dev/null | $(usp-sed) \
    -n -e 's|^[[:space:]]*SONAME[[:space:]]\+\([^[:space:]]\+\).*|\1|p' \
)

#
# $(call filter-files,<sopath>*)
#
filter-files = $(strip \
  $(foreach p,$1, \
    $(if $(call read-link,$p), \
      $(call REM,symbolic link) \
    , \
      $(call REM,assume regular file) \
      $p \
    ) \
  ) \
)

#
# $(call runtime-link-impl,<sofile>,<soname>?)
#
runtime-link-impl = $(strip \
  $(if $(filter-out $(notdir $1),$2), \
    $(call REM,sofile requires runtime link) \
    $2 \
  ) \
)
    
#
# $(call runtime-link,<sofile>)
# Returns the name (if any) of a sofile's required runtime link
#
runtime-link = $(call runtime-link-impl,$1,$(call soname,$1))

#
# $(call filter-devlinks,<sopath>*)
#
filter-devlinks = $(strip \
  $(foreach p,$1, \
    $(if $(call read-link,$p), \
      $(if $(call runtime-link,$p), \
        $(call REM,sopath is not a runtime link) \
        $p \
      ) \
    ) \
  ) \
)
        
#
# $(call devel-link-from-soname,<soname>,<candidate link>*)
#
devel-link-from-soname = $(strip \
  $(if $(firstword $2), \
    $(if $(filter $1,$(call soname,$(firstword $2))), \
      $(call REM,soname of target and candidate link match) \
      $(notdir $(firstword $2)) \
    , \
      $(call REM,try next link) \
      $(call devel-link-from-soname,$1,$(wordlist 2,$(words $2),$2)) \
    ) \
  ) \
)

#
# $(call devel-link,<sofile>,<candidate link>*)
# Returns the name (if any) of a sofile's development link
#
devel-link = $(call devel-link-from-soname,$(call soname,$1),$2)

#
# $(call pick-sofile-libfilename,<sofile>,<soname>?,<devel link name>?)
#
pick-sofile-libfilename = $(strip \
  $(if $(strip $3), \
    $(call REM,use devel link name) \
    $3 \
  , $(if $(strip $2), \
      $(call REM,fall back to soname) \
      $2 \
    , \
      $(call REM,fall back to filename) \
      $(notdir $1) \
    ) \
  ) \
)

#
# $(call sofile-libfilename-impl,<sofile>,<soname>?,<candidate link>*)
#
sofile-libfilename-impl = $(strip \
  $(call pick-sofile-libfilename,$1,$2,$(call devel-link-from-soname,$2,$3)) \
)
     
#
# $(call sofile-libfilename,<sofile>,<candidate link>*)
#
sofile-libfilename = $(call sofile-libfilename-impl,$1,$(call soname,$1),$2)

#
# $(call libfile-libname,<libfile>)
#
libfile-libname = $(strip \
  $(if $(filter .a .dylib .lib .so,$(suffix $1)), \
    $(patsubst lib%,%,$(notdir $(basename $1))) \
  , \
    $(if $(suffix $1), \
      $(call libfile-libname,$(basename $1)) \
    , \
      $(error libfile-libname: no expected suffix found) \
    ) \
  ) \
)

#
# $(call sofile-libname,<sofile>,<candidate link>*)
#
sofile-libname = $(call libfile-libname,$(call sofile-libfilename,$1,$2))

override with-devel := $(if $(filter yes,$(with-devel)),yes,)

override dest-dir := $(call required-value,dest-dir)
override libs-dir := $(call required-value,libs-dir)
ifdef with-devel
  override hdrfiles := $(call find-files,%,$(call required-value,headers-dir))
  override libfiles := $(wildcard $(libs-dir)/$(if $(windows),*.lib,*.a))
else
  override hdrfiles :=
  override libfiles :=
endif

ifdef windows
  override sopaths :=
  override dylibfiles :=
  override dllfiles := $(wildcard $(libs-dir)/*.dll)
  override pdbfiles := $(wildcard $(libs-dir)/*.pdb)
else ifdef darwin
  override sopaths :=
  override dylibfiles := $(wildcard $(libs-dir)/*.dylib)
  override dllfiles :=
  override pdbfiles :=
else
  override sopaths := $(wildcard $(libs-dir)/*.so $(libs-dir)/*.so.*)
  override dylibfiles :=
  override dllfiles :=
  override pdbfiles :=
endif

#
# Get regular files and development links from sopaths
#
override sofiles := $(call filter-files,$(sopaths))
override devlinks := $(call filter-devlinks,$(sopaths))

#
# Determine target files
#
override dst-hdrfiles := $(addprefix $(dest-dir)/include/, \
  $(patsubst $(headers-dir)/%,%,$(hdrfiles)))
override dst-libfiles := $(addprefix $(dest-dir)/lib/, \
  $(patsubst $(libs-dir)/%,%,$(libfiles)))
override dst-sofiles := $(addprefix $(dest-dir)/lib/, \
  $(patsubst $(libs-dir)/%,%,$(sofiles)))
override dst-dylibfiles := $(addprefix $(dest-dir)/lib/, \
  $(patsubst $(libs-dir)/%,%,$(dylibfiles)))
override dst-dllfiles := $(addprefix $(dest-dir)/bin/, \
  $(patsubst $(libs-dir)/%,%,$(dllfiles)))
override dst-pdbfiles := $(addprefix $(dest-dir)/bin/, \
  $(patsubst $(libs-dir)/%,%,$(pdbfiles)))

#
# Determine target dirs
#
override dst-dirs := $(call dedup,$(patsubst %/,%,$(dir \
  $(dst-hdrfiles) $(dst-libfiles) $(dst-sofiles) \
  $(dst-dylibfiles) $(dst-dllfiles) $(dst-pdbfiles) \
)))

override jam-dir := $(dest-dir)/lib/jamfiles

.PHONY: all
all: \
  $(dst-hdrfiles) \
  $(dst-libfiles) \
  $(dst-sofiles) \
  $(dst-dylibfiles) \
  $(dst-dllfiles) \
  $(dst-pdbfiles)

$(dst-hdrfiles): | $(dst-dirs)

$(dst-hdrfiles): $(dest-dir)/include/%: $(headers-dir)/%
	$(usp-cp) "$(call to-shell,$<)" "$(call to-shell,$@)"

$(dst-libfiles): | $(dst-dirs)

$(dst-libfiles): $(dest-dir)/lib/%: $(libs-dir)/%
	$(usp-cp) "$(call to-shell,$<)" "$(call to-shell,$@)"
	$(MAKE) -I "$(usp-builder-include-dir)" \
	  -f "$(usp-builder-lib-dir)/UpdateStagedJamfile.mak" \
	  jamfile="$(jam-dir)/$(call libfile-libname,$<)/jamfile" \
	  libfile="$@" \
	  libname="$(call libfile-libname,$<)" \
	  incdir="$(dest-dir)/include"

$(dst-sofiles): | $(dst-dirs)

$(dst-sofiles): $(dest-dir)/lib/%: $(libs-dir)/%
	$(usp-cp) "$(call to-shell,$<)" "$(call to-shell,$@)"
	$(if $(call runtime-link,$<),$(usp-ln-sf) \
	  "$(call to-shell,$(notdir $@))" \
	  "$(call to-shell,$(dir $@)$(call runtime-link,$<))")
ifdef with-devel
	$(if $(call devel-link,$<,$(devlinks)),$(usp-ln-sf) \
	  "$(call to-shell,$(notdir $@))" \
	  "$(call to-shell,$(dir $@)$(call devel-link,$<,$(devlinks)))")
	$(MAKE) -I "$(usp-builder-include-dir)" \
	  -f "$(usp-builder-lib-dir)/UpdateStagedJamfile.mak" \
	  jamfile="$(jam-dir)/$(call sofile-libname,$<,$(devlinks))/jamfile" \
	  libfile="$(dir $@)$(call sofile-libfilename,$<,$(devlinks))" \
	  libname="$(call sofile-libname,$<,$(devlinks))" \
	  incdir="$(dest-dir)/include"
endif

$(dst-dylibfiles): | $(dst-dirs)

$(dst-dylibfiles): $(dest-dir)/lib/%: $(libs-dir)/%
	$(usp-cp) "$(call to-shell,$<)" "$(call to-shell,$@)"
ifdef with-devel
	$(MAKE) -I "$(usp-builder-include-dir)" \
	  -f "$(usp-builder-lib-dir)/UpdateStagedJamfile.mak" \
	  jamfile="$(jam-dir)/jamfiles/$(call libfile-libname,$<)/jamfile" \
	  libfile="$@" \
	  libname="$(call libfile-libname,$<)" \
	  incdir="$(dest-dir)/include"
endif

$(dst-dllfiles): | $(dst-dirs)

$(dst-dllfiles): $(dest-dir)/bin/%: $(libs-dir)/%
	$(usp-cp) "$(call to-shell,$<)" "$(call to-shell,$@)"

$(dst-pdbfiles): | $(dst-dirs)

$(dst-pdbfiles): $(dest-dir)/bin/%: $(libs-dir)/%
	$(usp-cp) "$(call to-shell,$<)" "$(call to-shell,$@)"

$(dst-dirs):
	$(usp-mkdir-p) "$(call to-shell,$@)"
