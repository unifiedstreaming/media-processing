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
License: FIXME$(if $(strip $5),$(newline)Requires: $(foreach p,$5,$p = %{version}-%{release}))
Provides: %{name} = %{version}

%description
$4

%prep

%build

%install$(foreach d,$8,$(newline)mkdir -p %{buildroot}$7/$d)$(foreach f,$9,$(newline)cp $6/$f %{buildroot}$7/$f)

%files$(foreach f,$9,$(newline)$7/$f)

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
