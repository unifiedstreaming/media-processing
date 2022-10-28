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

include usp-builder/USPPackaging.mki

#
# Set some derived variables
#

override main-zip-filename := $(package)_$(pkg-version)-$(pkg-revision).zip
override pdb-zip-filename := $(package)-pdb_$(pkg-version)-$(pkg-revision).zip

override artifacts := $(patsubst $(artifacts-dir)/$(package)/%,%,$(call find-files,%,$(artifacts-dir)/$(package)))

override main-artifacts := $(if $(with-symbol-pkg),$(filter-out %.pdb,$(artifacts)),$(artifacts))
override pdb-artifacts := $(if $(with-symbol-pkg),$(filter %.pdb,$(artifacts)),)

override empty-zip-file := $(usp-builder-lib-dir)/empty.zip

override zip-options := -X $(if $(windows),,--symlinks)

#
# Rules
#
.PHONY: all
all: zip-file

.PHONY: zip-file
zip-file: $(pkgs-dir)/$(main-zip-filename) \
  $(if $(with-symbol-pkg),$(pkgs-dir)/$(pdb-zip-filename))

$(pkgs-dir)/$(main-zip-filename): force | $(pkgs-dir)
	$(usp-rm-rf) "$(call to-shell,$@)"
	$(if $(main-artifacts),cd "$(call to-shell,$(artifacts-dir)/$(package))" && $(usp-zip) $(zip-options) "$(call to-shell,$@)" $(foreach a,$(main-artifacts),"$(call to-shell,$a)"),$(usp-cp) "$(call to-shell,$(empty-zip-file))" "$(call to-shell,$@)")
	
$(pkgs-dir)/$(pdb-zip-filename): force | $(pkgs-dir)
	$(usp-rm-rf) "$(call to-shell,$@)"
	$(if $(pdb-artifacts),cd "$(call to-shell,$(artifacts-dir)/$(package))" && $(usp-zip) $(zip-options) "$(call to-shell,$@)" $(foreach a,$(pdb-artifacts),"$(call to-shell,$a)"),$(usp-cp) "$(call to-shell,$(empty-zip-file))" "$(call to-shell,$@)")

$(pkgs-dir):
	$(usp-mkdir-p) "$(call to-shell,$@)"

.PHONY: force
force:
