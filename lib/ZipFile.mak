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
# Avoid confusing messages from zip when run in parallel
#
.NOTPARALLEL:

include include/USPPackaging.mki

#
# Set some derived variables
#

override main-zip-basename := $(package)$(build-settings-suffix)_$(pkg-version)-$(pkg-revision)
override pdb-zip-basename := $(package)$(build-settings-suffix)-pdb_$(pkg-version)-$(pkg-revision)

override work-dir := $(packaging-work-dir)/zip/$(main-zip-basename)
override main-work-dir := $(work-dir)/main
override pdb-work-dir := $(work-dir)/pdb

override artifacts := $(patsubst $(artifacts-dir)/$(package)/%,%,$(call find-files,%,$(artifacts-dir)/$(package)))

override main-artifacts := $(addprefix $(main-work-dir)/,$(if $(with-symbol-pkg),$(filter-out %.pdb,$(artifacts)),$(artifacts)))
override pdb-artifacts := $(addprefix $(pdb-work-dir)/,$(if $(with-symbol-pkg),$(filter %.pdb,$(artifacts)),))

override main-dirs := $(call dedup,$(patsubst %/,%,$(dir $(main-artifacts))))
override pdb-dirs := $(call dedup,$(patsubst %/,%,$(dir $(pdb-artifacts))))

#
# Rules
#
.PHONY: all
all: zip-file

.PHONY: zip-file
zip-file: $(pkgs-dir)/$(main-zip-basename).zip \
  $(if $(with-symbol-pkg),$(pkgs-dir)/$(pdb-zip-basename).zip)

$(pkgs-dir)/$(main-zip-basename).zip: $(main-work-dir) $(main-artifacts) \
  | $(pkgs-dir)
	$(usp-rm-rf) "$(call to-shell,$@)"
	cd "$(call to-shell,$(main-work-dir))" && $(usp-zip) -X -r "$(call to-shell,$@)" . $(if $(main-artifacts),,-i .)
	
$(pkgs-dir)/$(pdb-zip-basename).zip: $(pdb-work-dir) $(pdb-artifacts) \
  | $(pkgs-dir)
	$(usp-rm-rf) "$(call to-shell,$@)"
	cd "$(call to-shell,$(pdb-work-dir))" && $(usp-zip) -X -r "$(call to-shell,$@)" . $(if $(pdb-artifacts),,-i .)

$(main-artifacts): $(main-work-dir)/%: $(artifacts-dir)/$(package)/% \
  $(main-dirs)
	$(usp-cp) "$(call to-shell,$<)" "$(call to-shell,$@)"

$(pdb-artifacts): $(pdb-work-dir)/%: $(artifacts-dir)/$(package)/% \
  $(pdb-dirs)
	$(usp-cp) "$(call to-shell,$<)" "$(call to-shell,$@)"

$(main-dirs): $(main-work-dir)
	$(usp-mkdir-p) "$(call to-shell,$@)"
	
$(pdb-dirs): $(pdb-work-dir)
	$(usp-mkdir-p) "$(call to-shell,$@)"
	
$(main-work-dir) $(pdb-work-dir): clean-work-dir
	$(usp-mkdir-p) "$(call to-shell,$@)"

$(pkgs-dir):
	$(usp-mkdir-p) "$(call to-shell,$@)"
	
.PHONY: clean-work-dir
clean-work-dir:
	$(usp-rm-rf) "$(call to-shell,$(work-dir))"
	$(usp-mkdir-p) "$(call to-shell,$(work-dir))"
