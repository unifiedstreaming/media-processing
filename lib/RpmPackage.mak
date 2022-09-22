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
# $(call is-installed,<package>)
#
is-installed = $(shell rpm -q "$1" >/dev/null 2>&1 && echo yes)

#
# $(call check-package-not-installed,<package>)
#
check-package-not-installed = $(strip \
  $(if $(call is-installed,$1), \
    $(error package "$1" appears to be installed - please purge it first) \
  ) \
)
  
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
# Get system-provided settings
#
override rpm-arch := $(call get-rpm-arch)

#
# Check that the package is not installed, as that may interfere with
# automatic shared library dependency detection from depending packages
#
$(call check-package-not-installed,$(package)$(build-settings-suffix))

#
# Set some derived variables
#
override rpm-package-basename := $(package)$(build-settings-suffix)-$(pkg-version)-$(pkg-revision).$(rpm-arch)

override rpm-work-dir := $(packaging-work-dir)/rpm/$(rpm-package-basename)

override artifacts := $(patsubst $(artifacts-dir)/$(package)/%,%,$(call find-files,%,$(artifacts-dir)/$(package)))

#
# $(call spec-file-content,<package>,<version>,<revision>,<description>,<license>,<prereq-package>*,<source artifact dir>,<source artifact>*,<service file template>*)
#
# Please note that, in contrast to the way Debian handles things, the default
# rpm systemd postinstall hook does not enable or any service; it only
# does a preset.
#
# See https://fedoraproject.org/wiki/Packaging:Systemd and
# https://docs.fedoraproject.org/en-US/packaging-guidelines/Scriptlets/#Systemd
#
define spec-file-content =
%define _build_id_links none
%global debug_package %{nil}
%global __os_install_post %{nil}

Name: $1
Version: $2
Release: $3
Summary: $4
License: $5
$(if $(strip $9),BuildRequires: systemd$(comma) systemd-rpm-macros)
$(call requires-line,$6)
Provides: %{name} = %{version}

%description
$4

%prep

%build

%install
$(foreach d,$(call distro-dirs,$8),$(newline)$(usp-mkdir-p) "%{buildroot}/$(call to-shell,$d)")
$(foreach a,$8,$(newline)$(usp-cp) "$(call to-shell,$7/$a)" "%{buildroot}/$(call to-shell,$(call distro-path,$a))")
$(if $(strip $9),$(newline)$(usp-mkdir-p) "%{buildroot}%{_unitdir}")
$(foreach t,$9,$(newline)$(usp-sed) 's/@BSS@/$(build-settings-suffix)/g' "$(call to-shell,$t)" >"%{buildroot}%{_unitdir}/$(call to-shell,$(call service-name,$t).service)")

%post
$(foreach t,$(service-file-templates),$(newline)%systemd_post $(call service-name,$t).service)

%preun
$(foreach t,$(service-file-templates),$(newline)%systemd_preun $(call service-name,$t).service)

%postun
$(foreach t,$(service-file-templates),$(newline)%systemd_postun_with_restart $(call service-name,$t).service)

%files
$(foreach a,$8,$(newline)$(if $(call filter-config-artifacts,$a),%config(noreplace) )/$(call distro-path,$a))$(foreach t,$9,$(newline)%{_unitdir}/$(call service-name,$t).service)

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
	$(file >$@,$(call spec-file-content,$(package)$(build-settings-suffix),$(pkg-version),$(pkg-revision),$(pkg-description),$(license),$(addsuffix $(build-settings-suffix),$(prereq-packages)),$(artifacts-dir)/$(package),$(artifacts),$(service-file-templates)))
	$(info generated $@)

$(rpm-work-dir)/SPECS: clean-rpm-work-dir
	$(usp-mkdir-p) "$(call to-shell,$@)"

.PHONY: clean-rpm-work-dir
clean-rpm-work-dir:
	$(usp-rm-rf) "$(call to-shell,$(rpm-work-dir))"
	$(usp-mkdir-p) "$(call to-shell,$(rpm-work-dir))"
