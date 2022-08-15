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

include include/USPPackaging.mki

#
# $(call checked-rpm-arch-output,<output>)
#
checked-rpm-arch-output = $(strip \
  $(if $(filter-out 1,$(words $1)), \
    $(error rpm failed to report architecture) \
  ) \
  $1 \
)

#
# $(call get-rpm-arch)
#
get-rpm-arch = $(call checked-rpm-arch-output, \
  $(shell rpm --eval '%{_arch}') \
)

#
# $(call requires-line-elt,<prereq package>)
#
requires-line-elt = $1 = %{version}-%{release}

#
# $(call requires-line-cont,<prereq package>*)
#
requires-line-cont = $(if $(firstword $1),$(comma) $(call requires-line-elt,$(firstword $1))$(call requires-line-cont,$(wordlist 2,$(words $1),$1)))

#
# $(call requires-line,<prereq package>*)
#
requires-line = $(if $(firstword $1),Requires: $(call requires-line-elt,$(firstword $1))$(call requires-line-cont,$(wordlist 2,$(words $1),$1)),# No explicit Requires: line)

#
# $(call directory-install-lines-elt,<installation prefix>,<directory>)
#
directory-install-lines-elt = $(usp-mkdir-p) "%{buildroot}$(call to-shell,$1/$2)"

#
# $(call directory-install-lines-cont,<installation prefix>,<directory>*)
#
directory-install-lines-cont = $(if $(firstword $2),$(newline)$(call directory-install-lines-elt,$1,$(firstword $2))$(call directory-install-lines-cont,$1,$(wordlist 2,$(words $2),$2))) 

#
# $(call directory-install-lines,<installation prefix>,<directory>*)
#
directory-install-lines = $(if $(firstword $2),$(call directory-install-lines-elt,$1,$(firstword $2))$(call directory-install-lines-cont,$1,$(wordlist 2,$(words $2),$2)),# No directories to install) 

#
# $(call file-install-lines-elt,<source dir>,<installation prefix>,<file>)
#
file-install-lines-elt = $(usp-cp) "$(call to-shell,$1/$3)" "%{buildroot}$(call to-shell,$2/$3)"

#
# $(call file-install-lines-cont,<source dir>,<installation prefix>,<file>*)
#
file-install-lines-cont = $(if $(firstword $3),$(newline)$(call file-install-lines-elt,$1,$2,$(firstword $3))$(call file-install-lines-cont,$1,$2,$(wordlist 2,$(words $3),$3)))

#
# $(call file-install-lines,<source-dir>,<installation prefix>,<file>*)
#
file-install-lines = $(if $(firstword $3),$(call file-install-lines-elt,$1,$2,$(firstword $3))$(call file-install-lines-cont,$1,$2,$(wordlist 2,$(words $3),$3)),# No files to install)

#
# $(call file-listing-lines-elt,<installation prefix>,<file>)
#
file-listing-lines-elt = $1/$2

#
# $(call file-listing-lines-cont,<installation prefix>,<file>*)
#
file-listing-lines-cont = $(if $(firstword $2),$(newline)$(call file-listing-lines-elt,$1,$(firstword $2))$(call file-listing-lines-cont,$1,$(wordlist 2,$(words $2),$2)))

#
# $(call file-listing-lines,<installation-prefux,<file>*)
#
file-listing-lines = $(if $(firstword $2),$(call file-listing-lines-elt,$1,$(firstword $2))$(call file-listing-lines-cont,$1,$(wordlist 2,$(words $2),$2)),# No files to package)

#
# Get system-provided settings
#
override rpm-arch := $(call get-rpm-arch)

#
# Set some derived variables
#
override rpm-package-basename := $(package)$(build-settings-suffix)-$(pkg-version)-$(pkg-revision).$(rpm-arch)

override rpm-work-dir := $(packaging-work-dir)/rpm/$(rpm-package-basename)

override installed-files := $(patsubst $(artifacts-dir)/$(package)/%,%,$(call find-files,%,$(artifacts-dir)/$(package)))
override installed-dirs := $(call dedup,$(patsubst %/,%,$(dir $(installed-files))))

#
# $(call spec-file-content,<package>,<version>,<revision>,<description>,<prereq-package>*,<source-dir>,<installation-prefix>,<installed-dir>*,<installed-file>*)
#
define spec-file-content =
%define _build_id_links none
%global debug_package %{nil}
%global __os_install_post %{nil}

Name: $1
Version: $2
Release: $3
Summary: $4
License: FIXME
$(call requires-line,$5)
Provides: %{name} = %{version}

%description
$4

%prep

%build

%install
$(call directory-install-lines,$7,$8)
$(call file-install-lines,$6,$7,$9)

%files
$(call file-listing-lines,$7,$9)

endef

.PHONY: all
all: rpm-package

.PHONY: rpm-package
rpm-package: $(pkgs-dir)/$(rpm-package-basename).rpm

$(pkgs-dir)/$(rpm-package-basename).rpm: $(rpm-work-dir)/RPMS/$(rpm-arch)/$(rpm-package-basename).rpm | $(pkgs-dir)
	$(usp-cp) "$(call to-shell,$<)" "$(call to-shell,$@)"

$(pkgs-dir) :
	$(usp-mkdir-p) "$(call to-shell,$@)"

$(rpm-work-dir)/RPMS/$(rpm-arch)/$(rpm-package-basename).rpm: $(rpm-work-dir)/SPECS/$(package)$(build-settings-suffix).spec
	rpmbuild -bb --define '_topdir $(rpm-work-dir)' $<

$(rpm-work-dir)/SPECS/$(package)$(build-settings-suffix).spec: $(rpm-work-dir)/SPECS
	$(file >$@,$(call spec-file-content,$(package)$(build-settings-suffix),$(pkg-version),$(pkg-revision),$(pkg-description),$(addsuffix $(build-settings-suffix),$(prereq-packages)),$(artifacts-dir)/$(package),$(installation-prefix),$(installed-dirs),$(installed-files)))
	$(info generated $@)

$(rpm-work-dir)/SPECS: clean-rpm-work-dir
	$(usp-mkdir-p) "$(call to-shell,$@)"

.PHONY: clean-rpm-work-dir
clean-rpm-work-dir:
	$(usp-rm-rf) "$(call to-shell,$(rpm-work-dir))"
	$(usp-mkdir-p) "$(call to-shell,$(rpm-work-dir))"
