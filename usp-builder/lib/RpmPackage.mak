#
# Copyright (C) 2022-2023 CodeShop B.V.
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

include usp-builder/USPPackaging.mki

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
# $(call service-post,<service name>)
# (stolen form systemd-rpm-macros package which is not available on Centos7)
#
define service-post =

if [ $$1 -eq 1 ] ; then
  # Initial installation
  systemctl --no-reload preset $1.service &>/dev/null || :
fi

endef

#
# $(call service-preun,<service name>
# (stolen form systemd-rpm-macros package which is not available on Centos7)
#
define service-preun =

if [ $$1 -eq 0 ] ; then 
  # Package removal, not upgrade 
  systemctl --no-reload disable --now $1.service &>/dev/null || : 
fi

endef

#
# $(call service-postun,<service name>
# (stolen form systemd-rpm-macros package which is not available on Centos7)
#
define service-postun =

if [ $$1 -ge 1 ] ; then 
  # Package upgrade, not uninstall 
  systemctl try-restart $1.service &>/dev/null || : 
fi

endef

#
# $(call checked-rpm-package-name-impl,<package name>,<char>*)
#
# https://docs.fedoraproject.org/en-US/packaging-guidelines/Naming/
# we allow for underscore (httpd mod_xyz) and uppercase (python modules)
# 
checked-rpm-package-name-impl = $(strip \
  $(if $(filter-out $(alphas) $(digits),$(firstword $2)), \
    $(error $(strip \
      package name '$1' must start with a letter or digit \
    )) \
  ) \
  $(if $(filter 0 1,$(words $2)), \
    $(error $(strip \
      package name '$1' must have at least two characters \
    )) \
  ) \
  $(if $(filter-out $(alphas) $(digits) - _,$(wordlist 2,$(words $2),$2)), \
    $(error $(strip \
      package name '$1' may only contain letters, digits, \
        '-' and '_' \
    )) \
  ) \
  $1 \
)

#
# $(call checked-rpm-package-name,<package name>)
#
checked-rpm-package-name = $(strip \
  $(if $(filter-out 1,$(words $1)), \
    $(error package name '$1' must have exactly one word) \
  ) \
  $(call checked-rpm-package-name-impl,$(strip $1), \
    $(call split-word,$(strip $1))) \
)

override package := $(call checked-rpm-package-name,$(package))

#
# Get system-provided settings
#
override rpm-arch := $(call get-rpm-arch)

#
# Check that the package is not installed, as that may interfere with
# automatic shared library dependency detection from depending packages
#
$(call check-package-not-installed,$(package))

#
# Set some derived variables
#
override rpm-package-basename := $(package)-$(pkg-version)-$(pkg-revision).$(rpm-arch)

override rpm-package-filename := $(rpm-package-basename).rpm

override rpm-debug-package-filename := $(package)-debuginfo-$(pkg-version)-$(pkg-revision).$(rpm-arch).rpm

override rpm-work-dir := $(packaging-work-dir)/$(rpm-package-basename)

override artifacts := $(patsubst $(artifacts-dir)/%,%,$(call find-files,%,$(artifacts-dir)))

#
# $(call spec-file-content,<package>,<version>,<revision>,<description>,<license>,<prereq-package>*,<source artifact dir>,<source artifact>*,<conf file>,<doc file>*,<service file>*,<apache conf file>*)
#
# Please note that, in contrast to the way Debian handles things, the
# default rpm systemd postinstall hook does not enable or start any
# service; it only does a preset.
#
# See https://fedoraproject.org/wiki/Packaging:Systemd and
# https://docs.fedoraproject.org/en-US/packaging-guidelines/Scriptlets/#Systemd
#
define spec-file-content =

$(if $(with-symbol-pkg),,%define _build_id_links none)
$(if $(with-symbol-pkg),,%global debug_package %{nil})
$(if $(with-symbol-pkg),,%global __os_install_post %{nil})

$(if $(with-symbol-pkg),%define _debugsource_template %{nil})

Name: $1
Version: $2
Release: $3
Summary: $4
License: $5
$(if $(strip $(11)),BuildRequires: systemd$(newline)%{?systemd_requires})
$(call requires-line,$6)
Provides: %{name} = %{version}

%description
$4

$(if $(with-symbol-pkg),%debug_package)

%prep

%build

%install

# Binaries
$(foreach d,$(call distro-dirs,$8),$(newline)$(usp-mkdir-p) "%{buildroot}/$(call to-shell,$d)")
$(foreach a,$8,$(newline)$(if $(call read-link,$7/$a),$(usp-ln-sf) "$(call to-shell,$(call read-link,$7/$a))" "%{buildroot}/$(call to-shell,$(call distro-path,$a))",$(usp-cp) "$(call to-shell,$7/$a)" "%{buildroot}/$(call to-shell,$(call distro-path,$a))"))

# Config files
$(if $(strip $9),$(newline)$(usp-mkdir-p) "%{buildroot}$(call to-shell,/etc)")
$(foreach f,$9,$(newline)$(usp-cp) "$(call to-shell,$f)" "%{buildroot}$(call to-shell,/etc/$(notdir $f))")

# Documentation files
$(if $(strip $(10)),$(newline)$(usp-mkdir-p) "%{buildroot}$(call to-shell,/usr/share/doc/$1)")
$(foreach f,$(10),$(newline)$(usp-cp) "$(call to-shell,$f)" "%{buildroot}$(call to-shell,/usr/share/doc/$1/$(notdir $f))")

# Unit files
$(if $(strip $(11)),$(newline)$(usp-mkdir-p) "%{buildroot}%{_unitdir}")
$(foreach f,$(11),$(newline)$(usp-cp) "$(call to-shell,$f)" "%{buildroot}%{_unitdir}/$(call to-shell,$(call service-name,$f)$(call service-suffix,$f))")

# Apache conf files
$(if $(strip $(12)),$(newline)$(usp-mkdir-p) "%{buildroot}%{_httpd_confdir}")
$(foreach f,$(12),$(newline)$(call subst-or-copy-apache-conf,$f,%{buildroot}%{_httpd_confdir},15-))

%post
$(foreach f,$(11),$(call service-post,$(call service-name,$f)))

%preun
$(foreach f,$(11),$(call service-preun,$(call service-name,$f)))

%postun
$(foreach f,$(11),$(call service-postun,$(call service-name,$f)))

%files
$(foreach f,$8,$(newline)/$(call distro-path,$f))$(foreach f,$9,$(newline)%config(noreplace) /etc/$(notdir $f))$(foreach f,$(10),$(newline)/usr/share/doc/$1/$(notdir $f))$(foreach f,$(11),$(newline)%{_unitdir}/$(call service-name,$f)$(call service-suffix,$f))$(foreach f,$(12),$(newline)%config(noreplace) %{_httpd_confdir}/$(call installed-apache-conf-file-name,$f,15-))

endef

.PHONY: all
all: rpm-package

.PHONY: rpm-package
rpm-package: $(pkgs-dir)/$(rpm-package-filename)

$(pkgs-dir)/$(rpm-package-filename): $(rpm-work-dir)/RPMS/$(rpm-arch)/$(rpm-package-filename) | $(pkgs-dir)
ifdef with-symbol-pkg
	$(usp-cp) "$(call to-shell,$(rpm-work-dir)/RPMS/$(rpm-arch)/$(rpm-debug-package-filename))" "$(call to-shell,$(pkgs-dir)/$(rpm-debug-package-filename))"
endif
	$(usp-cp) "$(call to-shell,$(rpm-work-dir)/RPMS/$(rpm-arch)/$(rpm-package-filename))" "$(call to-shell,$(pkgs-dir)/$(rpm-package-filename))"

$(pkgs-dir) :
	$(usp-mkdir-p) "$(call to-shell,$@)"

$(rpm-work-dir)/RPMS/$(rpm-arch)/$(rpm-package-filename): $(rpm-work-dir)/SPECS/$(package).spec
	rpmbuild -bb --define '_topdir $(rpm-work-dir)' $<

$(rpm-work-dir)/SPECS/$(package).spec: $(rpm-work-dir)/SPECS
	$(file >$@,$(call spec-file-content,$(package),$(pkg-version),$(pkg-revision),$(pkg-description),$(license),$(addsuffix ,$(prereq-packages)),$(artifacts-dir),$(artifacts),$(conf-files),$(doc-files),$(service-files),$(apache-conf-files)))
	$(info generated $@)

$(rpm-work-dir)/SPECS: clean-rpm-work-dir
	$(usp-mkdir-p) "$(call to-shell,$@)"

.PHONY: clean-rpm-work-dir
clean-rpm-work-dir:
	$(usp-rm-rf) "$(call to-shell,$(rpm-work-dir))"
	$(usp-mkdir-p) "$(call to-shell,$(rpm-work-dir))"
