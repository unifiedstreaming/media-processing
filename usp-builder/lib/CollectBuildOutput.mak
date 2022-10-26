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
# $(call required-runtime-link,<sofile>)
#
required-runtime-link = $(filter-out $(notdir $1),$(call get-soname,$1))

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
# $(call required-devel-link,<sofile>)
#
required-devel-link = $(filter-out $(notdir $1),$(call get-develname,$1))
      
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

override headers := $(if $(with-devel),$(patsubst $(headers-dir)/%,%,$(call find-files,%,$(call required-value,headers-dir))))
override installed-headers := $(addprefix $(dest-dir)/include/,$(headers))
override installed-header-dirs := $(call dedup,$(patsubst %/,%,$(dir $(installed-headers))))

override sofiles := $(if $(windows),,$(notdir $(shell find "$(libs-dir)" -maxdepth 1 -type f '(' -name "*.so" -o -name "*.so.*" ')')))
override installed-sofiles := $(addprefix $(dest-dir)/lib/,$(sofiles))

override dllfiles := $(if $(windows),$(notdir $(wildcard $(libs-dir)/*.dll)))
override installed-dllfiles := $(addprefix $(dest-dir)/bin/,$(dllfiles))

override pdbfiles := $(if $(windows),$(notdir $(wildcard $(libs-dir)/*.pdb)))
override installed-pdbfiles := $(addprefix $(dest-dir)/bin/,$(pdbfiles))

override libfiles := $(if $(with-devel),$(notdir $(wildcard $(libs-dir)/$(if $(windows),*.lib,*.a))))
override installed-libfiles := $(addprefix $(dest-dir)/lib/,$(libfiles))

.PHONY: all
all: $(installed-headers) $(installed-sofiles) $(installed-dllfiles) $(installed-pdbfiles) $(installed-libfiles)

$(installed-headers): | $(installed-header-dirs)

$(installed-headers): $(dest-dir)/include/%: $(headers-dir)/%
	$(usp-cp) "$(call to-shell,$<)" "$(call to-shell,$@)"

$(installed-sofiles): | $(dest-dir)/lib

$(installed-sofiles): $(dest-dir)/lib/%: $(libs-dir)/%
	$(usp-cp) "$(call to-shell,$<)" "$(call to-shell,$@)"
	$(if $(call required-runtime-link,$<),ln -sf \
		"$(call to-shell,$(notdir $@))" \
		"$(call to-shell,$(dir $@)$(call required-runtime-link,$<))")
	$(if $(and $(with-devel),$(call required-devel-link,$<)),ln -sf \
		"$(call to-shell,$(notdir $@))" \
		"$(call to-shell,$(dir $@)$(call required-devel-link,$<))")
	$(if $(with-devel),$(MAKE) -I "$(usp-builder-include-dir)" \
	  -f "$(usp-builder-lib-dir)/UpdateStagedJamfile.mak" \
	  jamfile="$(dest-dir)/lib/jamfiles/$(call get-sofile-libname,$<)/jamfile" \
	  libfile="$(dest-dir)/lib/$(call get-develname,$<)" \
	  libname="$(call get-sofile-libname,$<)" \
	  incdir="$(dest-dir)/include")

$(installed-dllfiles): | $(dest-dir)/bin

$(installed-dllfiles): $(dest-dir)/bin/%: $(libs-dir)/%
	$(usp-cp) "$(call to-shell,$<)" "$(call to-shell,$@)"

$(installed-pdbfiles): | $(dest-dir)/bin

$(installed-pdbfiles): $(dest-dir)/bin/%: $(libs-dir)/%
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
	
$(installed-header-dirs) $(dest-dir)/bin $(dest-dir)/lib:
	$(usp-mkdir-p) "$(call to-shell,$@)"
