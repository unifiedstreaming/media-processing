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
# $(call get-soname,<sofile>)
#
get-soname = $(shell objdump -p "$(call to-shell,$1)" 2>/dev/null | awk '/^ *SONAME +/ { print $$2 }')

#
# $(call runtime-link,<sofile>)
#
runtime-link = $(filter-out $(notdir $1),$(call get-soname,$1))

#
# $(call get-develname,<sofile>)
#
get-develname = $(strip \
  $(if $(suffix $1), \
    $(if $(filter .so,$(suffix $1)), \
      $(notdir $1) \
    , \
      $(call get-develname,$(basename $1)) \
    ) \
  ) \
)

#
# $(call devel-link,<sofile>)
#
devel-link = $(filter-out $(notdir $1),$(call get-develname,$1))
      
#
# $(call get-sofile-libname,<sofile>)
#
get-sofile-libname = $(patsubst lib%,%,$(basename $(call get-develname,$1)))

#
# $(call get-libfile-libname,<libfile>)
#
get-libfile-libname = $(patsubst lib%,%,$(call notdir-basename,$1))

override with-devel := $(if $(filter yes,$(with-devel)),yes,)

override dest-dir := $(call required-value,dest-dir)
override libs-dir := $(call required-value,libs-dir)

ifdef with-devel
  override headers := $(patsubst $(headers-dir)/%,%,$(call find-files,%,$(call required-value,headers-dir)))
  override libfiles := $(notdir $(wildcard $(libs-dir)/$(if $(windows),*.lib,*.a)))
else
  override headers :=
  override libflles := 
endif

override installed-headers := $(addprefix $(dest-dir)/include/,$(headers))
override installed-header-dirs := $(call dedup,$(patsubst %/,%,$(dir $(installed-headers))))
override installed-libfiles := $(addprefix $(dest-dir)/lib/,$(libfiles))

ifdef windows
  override sofiles :=
  override dylibfiles := 
  override dllfiles := $(notdir $(wildcard $(libs-dir)/*.dll))
  override pdbfiles := $(notdir $(wildcard $(libs-dir)/*.pdb))
else ifdef darwin
  override sofiles :=
  override dylibfiles := $(notdir $(wildcard $(libs-dir)/*.dylib))
  override dllfiles :=
  override pdbfiles :=
else
  override sofiles := $(notdir $(shell find "$(call to-shell,$(libs-dir))" -maxdepth 1 -type f '(' -name "*.so" -o -name "*.so.*" ')'))
  override dylibfiles := 
  override dllfiles :=
  override pdbfiles :=
endif

override installed-sofiles := $(addprefix $(dest-dir)/lib/,$(sofiles))
override installed-dylibfiles := $(addprefix $(dest-dir)/lib/,$(dylibfiles))
override installed-dllfiles := $(addprefix $(dest-dir)/bin/,$(dllfiles))
override installed-pdbfiles := $(addprefix $(dest-dir)/bin/,$(pdbfiles))

.PHONY: all
all: $(installed-headers) \
  $(installed-libfiles) \
  $(installed-sofiles) \
  $(installed-dylibfiles) \
  $(installed-dllfiles) \
  $(installed-pdbfiles)

$(installed-headers): | $(installed-header-dirs)

$(installed-headers): $(dest-dir)/include/%: $(headers-dir)/%
	$(usp-cp) "$(call to-shell,$<)" "$(call to-shell,$@)"

$(installed-libfiles): | $(dest-dir)/lib

$(installed-libfiles): $(dest-dir)/lib/%: $(libs-dir)/%
	$(usp-cp) "$(call to-shell,$<)" "$(call to-shell,$@)"
	$(MAKE) -I "$(usp-builder-include-dir)" \
	  -f "$(usp-builder-lib-dir)/UpdateStagedJamfile.mak" \
	  jamfile="$(dest-dir)/lib/jamfiles/$(call get-libfile-libname,$<)/jamfile" \
	  libfile="$@" \
	  libname="$(call get-libfile-libname,$<)" \
	  incdir="$(dest-dir)/include"
	
$(installed-sofiles): | $(dest-dir)/lib

$(installed-sofiles): $(dest-dir)/lib/%: $(libs-dir)/%
	$(usp-cp) "$(call to-shell,$<)" "$(call to-shell,$@)"
	$(if $(call runtime-link,$<),ln -sf \
	  "$(call to-shell,$(notdir $@))" \
	  "$(call to-shell,$(dir $@)$(call runtime-link,$<))")
ifdef with-devel
	$(if $(call devel-link,$<),ln -sf \
	  "$(call to-shell,$(notdir $@))" \
	  "$(call to-shell,$(dir $@)$(call devel-link,$<))")
	$(MAKE) -I "$(usp-builder-include-dir)" \
	  -f "$(usp-builder-lib-dir)/UpdateStagedJamfile.mak" \
	  jamfile="$(dest-dir)/lib/jamfiles/$(call get-sofile-libname,$<)/jamfile" \
	  libfile="$(dest-dir)/lib/$(call get-develname,$<)" \
	  libname="$(call get-sofile-libname,$<)" \
	  incdir="$(dest-dir)/include"
endif

$(installed-dylibfiles): | $(dest-dir)/lib

$(installed-dylibfiles): $(dest-dir)/lib/%: $(libs-dir)/%
	$(usp-cp) "$(call to-shell,$<)" "$(call to-shell,$@)"
ifdef with-devel
	$(MAKE) -I "$(usp-builder-include-dir)" \
	  -f "$(usp-builder-lib-dir)/UpdateStagedJamfile.mak" \
	  jamfile="$(dest-dir)/lib/jamfiles/$(call get-libfile-libname,$<)/jamfile" \
	  libfile="$@" \
	  libname="$(call get-libfile-libname,$<)" \
	  incdir="$(dest-dir)/include"
endif
	
$(installed-dllfiles): | $(dest-dir)/bin

$(installed-dllfiles): $(dest-dir)/bin/%: $(libs-dir)/%
	$(usp-cp) "$(call to-shell,$<)" "$(call to-shell,$@)"

$(installed-pdbfiles): | $(dest-dir)/bin

$(installed-pdbfiles): $(dest-dir)/bin/%: $(libs-dir)/%
	$(usp-cp) "$(call to-shell,$<)" "$(call to-shell,$@)"

$(installed-header-dirs) $(dest-dir)/bin $(dest-dir)/lib:
	$(usp-mkdir-p) "$(call to-shell,$@)"
