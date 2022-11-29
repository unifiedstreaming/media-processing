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
# $(call main-base,<package>,<version>,<revision>)
#
main-base = $1_$2-$3

#
# $(call pdb-base,<package>,<version>,<revision>)
#
pdb-base = $1-pdb_$2-$3

#
# $(call main-meta-content,<package>,<version>,<revision>,<prereq package>*)
#
define main-meta-content =
package: $(call main-base,$1,$2,$3).zip
requires: $(foreach p,$4,$(call main-base,$p,$2,$3).zip)

endef

#
# $(call pdb-meta-content,<package>,<version>,<revision>)
#
define pdb-meta-content =
package: $(call pdb-base,$1,$2,$3).zip
requires: $(call main-base,$1,$2,$3).zip

endef

#
# Set some derived variables
#
override main-zip-basename := $(call main-base,$(package),$(pkg-version),$(pkg-revision))
override pdb-zip-basename := $(call pdb-base,$(package),$(pkg-version),$(pkg-revision))

override main-zip-filename := $(main-zip-basename).zip
override pdb-zip-filename := $(pdb-zip-basename).zip

override artifacts := $(patsubst $(artifacts-dir)/%,%,$(call find-files,%,$(artifacts-dir)))

override main-artifacts := $(filter-out %.pdb,$(artifacts))
override main-artifact-dirs := $(call dedup,$(patsubst %/,%,$(dir $(main-artifacts))))

override pdb-artifacts := $(if $(with-symbol-pkg),$(filter %.pdb,$(artifacts)),)
override pdb-artifact-dirs := $(call dedup,$(patsubst %/,%,$(dir $(pdb-artifacts))))

override zip-options := -X $(if $(windows),,--symlinks)

override main-work-dir := $(packaging-work-dir)/$(main-zip-basename)
override pdb-work-dir := $(packaging-work-dir)/$(pdb-zip-basename)

#
# Rules
#
.PHONY: all
all: zip-file

.PHONY: zip-file
zip-file: $(pkgs-dir)/$(main-zip-filename) $(if $(with-symbol-pkg),$(pkgs-dir)/$(pdb-zip-filename))

$(pkgs-dir)/$(main-zip-filename): \
  $(main-work-dir)/usp-meta/$(main-zip-basename).zip.meta \
  clean-main-work-dir \
  | $(pkgs-dir)
	$(usp-rm-rf) "$(call to-shell,$@)"
	$(foreach d,$(main-artifact-dirs),$(usp-mkdir-p) "$(call to-shell,$(main-work-dir)/$d)"$(newline)$(tab))
	$(foreach a,$(main-artifacts),$(if $(call read-link,$(artifacts-dir)/$a),ln -sf "$(call read-link,$(artifacts-dir)/$a)" "$(call to-shell,$(main-work-dir)/$a)",$(usp-cp) "$(call to-shell,$(artifacts-dir)/$a)" "$(call to-shell,$(main-work-dir)/$a)")$(newline)$(tab))
	$(if $(strip $(conf-files)),$(usp-mkdir-p) "$(call to-shell,$(main-work-dir)/etc)"$(newline)$(tab))
	$(foreach f,$(conf-files),$(usp-cp) "$(call to-shell,$f)" "$(call to-shell,$(main-work-dir)/etc/$(notdir $f))"$(newline)$(tab))
	$(if $(strip $(doc-files)),$(usp-mkdir-p) "$(call to-shell,$(main-work-dir)/doc/$(package))"$(newline)$(tab))
	$(foreach f,$(doc-files),$(usp-cp) "$(call to-shell,$f)" "$(call to-shell,$(main-work-dir)/doc/$(package)/$(notdir $f))"$(newline)$(tab))
	( cd "$(call to-shell,$(main-work-dir))" && $(usp-zip) $(zip-options) -r "$(call to-shell,$@)" . )

$(main-work-dir)/usp-meta/$(main-zip-basename).zip.meta: \
  $(main-work-dir)/usp-meta
	$(file >$@,$(call main-meta-content,$(package),$(pkg-version),$(pkg-revision),$(prereq-packages)))
	$(info generated $@)
	
$(main-work-dir)/usp-meta: clean-main-work-dir
	$(usp-mkdir-p) "$(call to-shell,$@)"
	
.PHONY: clean-main-work-dir
clean-main-work-dir:
	$(usp-rm-rf) "$(call to-shell,$(main-work-dir))"
	$(usp-mkdir-p) "$(call to-shell,$(main-work-dir))"

$(pkgs-dir)/$(pdb-zip-filename): \
  $(pdb-work-dir)/usp-meta/$(pdb-zip-basename).zip.meta \
  clean-pdb-work-dir \
  | $(pkgs-dir)
	$(usp-rm-rf) "$(call to-shell,$@)"
	$(foreach d,$(pdb-artifact-dirs),$(usp-mkdir-p) "$(call to-shell,$(pdb-work-dir)/$d)"$(newline)$(tab))
	$(foreach a,$(pdb-artifacts),$(if $(call read-link,$(artifacts-dir)/$a),ln -sf "$(call read-link,$(artifacts-dir)/$a)" "$(call to-shell,$(pdb-work-dir)/$a)",$(usp-cp) "$(call to-shell,$(artifacts-dir)/$a)" "$(call to-shell,$(pdb-work-dir)/$a)")$(newline)$(tab))
	( cd "$(call to-shell,$(pdb-work-dir))" && $(usp-zip) $(zip-options) -r "$(call to-shell,$@)" . )

$(pdb-work-dir)/usp-meta/$(pdb-zip-basename).zip.meta: \
  $(pdb-work-dir)/usp-meta
	$(file >$@,$(call pdb-meta-content,$(package),$(pkg-version),$(pkg-revision)))
	$(info generated $@)

$(pdb-work-dir)/usp-meta: clean-pdb-work-dir
	$(usp-mkdir-p) "$(call to-shell,$@)"
	
.PHONY: clean-pdb-work-dir
clean-pdb-work-dir:
	$(usp-rm-rf) "$(call to-shell,$(pdb-work-dir))"
	$(usp-mkdir-p) "$(call to-shell,$(pdb-work-dir))"

$(pkgs-dir):
	$(usp-mkdir-p) "$(call to-shell,$@)"
