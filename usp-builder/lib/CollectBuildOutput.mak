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
    $(call REM,runtime link for somename required) \
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
        $(call REM,not a runtime link) \
        $p \
      ) \
    ) \
  ) \
)
        
#
# $(call devel-link-impl,<soname>,<candidate link>*)
#
devel-link-impl = $(strip \
  $(if $(firstword $2), \
    $(if $(filter $1,$(call soname,$(firstword $2))), \
      $(call REM,soname of target and link match) \
      $(notdir $(firstword $2)) \
    , \
      $(call REM,try next link) \
      $(call devel-link-impl,$1,$(wordlist 2,$(words $2),$2)) \
    ) \
  ) \
)

#
# $(call devel-link,<sofile>,<candidate link>*)
# Returns the name (if any) of a sofile's development link
#
devel-link = $(call devel-link-impl,$(call soname,$1),$2)

#
# $(call sofile-libfilename-impl,<sofile>,<link>?)
#
sofile-libfilename-impl = $(notdir $(if $2,$2,$1))

#
# $(call sofile-libfilename,<sofile>,<candidate link>*)
#
sofile-libfilename = $(call sofile-libfilename-impl,$1,$(call devel-link,$1,$2))

#
# $(call libfile-libname,<libfile>)
#
libfile-libname = $(strip \
  $(if $(filter %.a %.dylib %.lib %.so,$1), \
    $(patsubst lib%,%,$(notdir $(basename $1))) \
  , \
    $(call libfile-libname,$(basename $1)) \
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

override sofiles := $(call filter-files,$(sopaths))
override devlinks := $(call filter-devlinks,$(sopaths))

override dst-hdrfiles := $(addprefix $(dest-dir)/include/, \
  $(patsubst $(headers-dir)/%,%,$(hdrfiles)))
override dst-hdrdirs := \
  $(call dedup,$(patsubst %/,%,$(dir $(dst-hdrfiles))))

override dst-libfiles := $(addprefix $(dest-dir)/lib/,$(notdir $(libfiles)))
override dst-sofiles := $(addprefix $(dest-dir)/lib/,$(notdir $(sofiles)))
override dst-dylibfiles := $(addprefix $(dest-dir)/lib/,$(notdir $(dylibfiles)))
override dst-dllfiles := $(addprefix $(dest-dir)/bin/,$(notdir $(dllfiles)))
override dst-pdbfiles := $(addprefix $(dest-dir)/bin/,$(notdir $(pdbfiles)))

override dest-hdrdirs := \
  $(call dedup,$(patsubst %/,%,$(dir $(dest-hdrfiles))))

.PHONY: all
all: \
  $(dst-hdrfiles) \
  $(dst-libfiles) \
  $(dst-sofiles) \
  $(dst-dylibfiles) \
  $(dst-dllfiles) \
  $(dst-pdbfiles)

$(dst-hdrfiles): | $(dst-hdrdirs)

$(dst-hdrfiles): $(dest-dir)/include/%: $(headers-dir)/%
	$(usp-cp) "$(call to-shell,$<)" "$(call to-shell,$@)"

$(dst-libfiles): | $(dest-dir)/lib

$(dst-libfiles): $(dest-dir)/lib/%: $(libs-dir)/%
	$(usp-cp) "$(call to-shell,$<)" "$(call to-shell,$@)"
	$(MAKE) -I "$(usp-builder-include-dir)" \
	  -f "$(usp-builder-lib-dir)/UpdateStagedJamfile.mak" \
	  jamfile="$(dir $@)jamfiles/$(call libfile-libname,$<)/jamfile" \
	  libfile="$@" \
	  libname="$(call libfile-libname,$<)" \
	  incdir="$(dest-dir)/include"

$(dst-sofiles): | $(dest-dir)/lib

$(dst-sofiles): $(dest-dir)/lib/%: $(libs-dir)/%
	$(usp-cp) "$(call to-shell,$<)" "$(call to-shell,$@)"
	$(if $(call runtime-link,$<),ln -sf \
	  "$(call to-shell,$(notdir $@))" \
	  "$(call to-shell,$(dir $@)$(call runtime-link,$<))")
ifdef with-devel
	$(if $(call devel-link,$<,$(devlinks)),ln -sf \
	  "$(call to-shell,$(notdir $@))" \
	  "$(call to-shell,$(dir $@)$(call devel-link,$<,$(devlinks)))")
	$(MAKE) -I "$(usp-builder-include-dir)" \
	  -f "$(usp-builder-lib-dir)/UpdateStagedJamfile.mak" \
	  jamfile="$(dir $@)jamfiles/$(call sofile-libname,$<,$(devlinks))/jamfile" \
	  libfile="$(dir $@)$(call sofile-libfilename,$<,$(devlinks))" \
	  libname="$(call sofile-libname,$<,$(devlinks))" \
	  incdir="$(dest-dir)/include"
endif

$(dst-dylibfiles): | $(dest-dir)/lib

$(dst-dylibfiles): $(dest-dir)/lib/%: $(libs-dir)/%
	$(usp-cp) "$(call to-shell,$<)" "$(call to-shell,$@)"
ifdef with-devel
	$(MAKE) -I "$(usp-builder-include-dir)" \
	  -f "$(usp-builder-lib-dir)/UpdateStagedJamfile.mak" \
	  jamfile="$(dir $@)jamfiles/$(call libfile-libname,$<)/jamfile" \
	  libfile="$@" \
	  libname="$(call libfile-libname,$<)" \
	  incdir="$(dest-dir)/include"
endif

$(dst-dllfiles): | $(dest-dir)/bin

$(dst-dllfiles): $(dest-dir)/bin/%: $(libs-dir)/%
	$(usp-cp) "$(call to-shell,$<)" "$(call to-shell,$@)"

$(dst-pdbfiles): | $(dest-dir)/bin

$(dst-pdbfiles): $(dest-dir)/bin/%: $(libs-dir)/%
	$(usp-cp) "$(call to-shell,$<)" "$(call to-shell,$@)"

$(dst-hdrdirs) $(dest-dir)/bin $(dest-dir)/lib:
	$(usp-mkdir-p) "$(call to-shell,$@)"
