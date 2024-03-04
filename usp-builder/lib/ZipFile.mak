#
# Copyright (C) 2022-2024 CodeShop B.V.
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
package: $(call main-base,$1,$2,$3)
requires: $(foreach p,$4,$(call main-base,$p,$(call get-package-version,$p),$(call get-package-revision,$p)))

endef

#
# $(call pdb-meta-content,<package>,<version>,<revision>)
#
define pdb-meta-content =
package: $(call pdb-base,$1,$2,$3)
requires: $(call main-base,$1,$2,$3)

endef

#
# Set some derived variables
#
override main-zip-basename := $(call main-base,$(package),$(pkg-version),$(pkg-revision))
override pdb-zip-basename := $(call pdb-base,$(package),$(pkg-version),$(pkg-revision))

override main-zip-filename := $(main-zip-basename).zip
override pdb-zip-filename := $(pdb-zip-basename).zip

override artifacts := $(patsubst $(artifacts-dir)/%,%,$(call find-files-and-links,$(artifacts-dir)))

override main-artifacts := $(filter-out %.pdb,$(artifacts))
override main-artifact-dirs := $(call dedup,$(patsubst %/,%,$(dir $(main-artifacts))))

override pdb-artifacts := $(if $(add-debug-package),$(filter %.pdb,$(artifacts)),)
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
zip-file: $(pkgs-dir)/$(main-zip-filename) $(if $(add-debug-package),$(pkgs-dir)/$(pdb-zip-filename))

$(pkgs-dir)/$(main-zip-filename): \
  $(main-work-dir)/usp-meta/$(main-zip-basename).meta \
  clean-main-work-dir \
  | $(pkgs-dir)
	$(usp-rm-file) "$(call to-shell,$@)"
	$(foreach d,$(main-artifact-dirs),$(usp-mkdir-p) "$(call to-shell,$(main-work-dir)/$d)"$(newline)$(tab))
	$(foreach a,$(main-artifacts),$(if $(call read-link,$(artifacts-dir)/$a),$(usp-ln-sf) "$(call read-link,$(artifacts-dir)/$a)" "$(call to-shell,$(main-work-dir)/$a)",$(usp-cp) "$(call to-shell,$(artifacts-dir)/$a)" "$(call to-shell,$(main-work-dir)/$a)")$(newline)$(tab))
	$(if $(strip $(conf-files)),$(usp-mkdir-p) "$(call to-shell,$(main-work-dir)/etc)"$(newline)$(tab))
	$(foreach f,$(conf-files),$(usp-cp) "$(call to-shell,$f)" "$(call to-shell,$(main-work-dir)/etc/$(notdir $f))"$(newline)$(tab))
	$(if $(strip $(doc-files)),$(usp-mkdir-p) "$(call to-shell,$(main-work-dir)/doc/$(package))"$(newline)$(tab))
	$(foreach f,$(doc-files),$(usp-cp) "$(call to-shell,$f)" "$(call to-shell,$(main-work-dir)/doc/$(package)/$(notdir $f))"$(newline)$(tab))
	cd "$(call to-shell,$(main-work-dir))" && $(usp-zip) $(zip-options) -r "$(call to-shell,$@)" .

$(main-work-dir)/usp-meta/$(main-zip-basename).meta: \
  $(main-work-dir)/usp-meta
	$(file >$@,$(call main-meta-content,$(package),$(pkg-version),$(pkg-revision),$(prereq-packages)))
	$(info generated $@)
	
$(main-work-dir)/usp-meta: clean-main-work-dir
	$(usp-mkdir-p) "$(call to-shell,$@)"
	
.PHONY: clean-main-work-dir
clean-main-work-dir:
	$(usp-rm-dir) "$(call to-shell,$(main-work-dir))"
	$(usp-mkdir-p) "$(call to-shell,$(main-work-dir))"

$(pkgs-dir)/$(pdb-zip-filename): \
  $(pdb-work-dir)/usp-meta/$(pdb-zip-basename).meta \
  clean-pdb-work-dir \
  | $(pkgs-dir)
	$(usp-rm-file) "$(call to-shell,$@)"
	$(foreach d,$(pdb-artifact-dirs),$(usp-mkdir-p) "$(call to-shell,$(pdb-work-dir)/$d)"$(newline)$(tab))
	$(foreach a,$(pdb-artifacts),$(if $(call read-link,$(artifacts-dir)/$a),$(usp-ln-sf) "$(call read-link,$(artifacts-dir)/$a)" "$(call to-shell,$(pdb-work-dir)/$a)",$(usp-cp) "$(call to-shell,$(artifacts-dir)/$a)" "$(call to-shell,$(pdb-work-dir)/$a)")$(newline)$(tab))
	cd "$(call to-shell,$(pdb-work-dir))" && $(usp-zip) $(zip-options) -r "$(call to-shell,$@)" .

$(pdb-work-dir)/usp-meta/$(pdb-zip-basename).meta: \
  $(pdb-work-dir)/usp-meta
	$(file >$@,$(call pdb-meta-content,$(package),$(pkg-version),$(pkg-revision)))
	$(info generated $@)

$(pdb-work-dir)/usp-meta: clean-pdb-work-dir
	$(usp-mkdir-p) "$(call to-shell,$@)"
	
.PHONY: clean-pdb-work-dir
clean-pdb-work-dir:
	$(usp-rm-dir) "$(call to-shell,$(pdb-work-dir))"
	$(usp-mkdir-p) "$(call to-shell,$(pdb-work-dir))"

$(pkgs-dir):
	$(usp-mkdir-p) "$(call to-shell,$@)"
