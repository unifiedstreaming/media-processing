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

#
# Based on Vincent Bernat's 'Pragmatic Debian packaging'
#
# https://vincent.bernat.ch/en/blog/2019-pragmatic-debian-packaging
#
# Thanks Vincent!
#

include usp-builder/USPPackaging.mki

#
# $(call is-installed,<package>)
#
is-installed = $(shell dpkg -s "$1" >/dev/null 2>&1 && echo yes)

#
# $(call check-package-not-installed,<package>)
#
check-package-not-installed = $(strip \
  $(if $(call is-installed,$1), \
    $(error package "$1" appears to be installed - please purge it first) \
  ) \
)
  
#
# $(call checked-deb-architecture-output,<output>)
#
checked-dpkg-architecture-output = $(strip \
  $(if $(filter-out 1,$(words $1)), \
    $(error 'dpkg' failed to report architecture) \
  ) \
  $1 \
)

#
# $(call get-deb-arch)
#
get-deb-arch = $(call checked-dpkg-architecture-output, \
  $(shell dpkg --print-architecture) \
)

#
# $(call checked-which-make-output,<output>)
#
checked-which-make-output = $(strip \
  $(if $(filter-out 1,$(words $1)), \
    $(error 'which' failed to report path to $(MAKE)) \
  ) \
  $1 \
)

#
# $(call get-make)
#
get-make = $(call checked-which-make-output, \
  $(shell which $(MAKE)) \
)

#
# $(call changelog-content,<package name>,<package version>)
#
define changelog-content =
$1 ($2) UNRELEASED; urgency=medium

  * Fake Entry

 -- Joe Coder <joe@dev.null>  $(shell date -R)

endef

#
# $(call checked-deb-package-name,<name>)
#
checked-deb-package-name = $(call checked-package-name,$1)

override package := $(call checked-deb-package-name,$(package))

#
# $(call control-content,<package name>,<maintainer>,<description>,<package version>,<package revision>,<prereq package>*)
#
define control-content =
Source: $1
Build-Depends: debhelper (>= $(debhelper-version))
Maintainer: $2

Package: $1
Architecture: $(call get-deb-arch)
Description: $3
Depends: $(foreach p,$6,$p (= $4-$5),) $${misc:Depends}, $${shlibs:Depends}
Section: misc
Priority: optional

endef

#
# $(call make-artifact-dirs,<package>,<deb-work-dir>,<artifact>*)
#
make-artifact-dirs = $(foreach d,$(call distro-dirs,$3),$(newline)$(tab)$(usp-mkdir-p) "$(call to-shell,$2/debian/$1/$d)")

#
# $(call install-artifacts,<package>,<deb-work-dir>,<artifacts-dir>,<artifact>*)
#
install-artifacts = $(foreach a,$4,$(newline)$(tab)$(if $(call read-link,$3/$a),$(usp-ln-sf) "$(call read-link,$3/$a)" "$(call to-shell,$2/debian/$1/$(call distro-path,$a))",$(usp-cp) "$(call to-shell,$3/$a)" "$(call to-shell,$2/debian/$1/$(call distro-path,$a))"))

#
# $(call make-conf-dir,<package>,<deb-work-dir>,<conf-file>*)
#
make-conf-dir = $(if $(strip $3),$(newline)$(tab)$(usp-mkdir-p) "$(call to-shell,$2/debian/$1/etc)")

#
# $(call install-conf-files,<package>,<deb-work-dir>,<conf-file>*)
#
install-conf-files = $(foreach f,$3,$(newline)$(tab)$(usp-cp) "$(call to-shell,$f)" "$(call to-shell,$2/debian/$1/etc/$(notdir $f))")

#
# $(call make-doc-dir,<package>,<deb-work-dir>,<doc-file>*)
#
make-doc-dir = $(if $(strip $3),$(newline)$(tab)$(usp-mkdir-p) "$(call to-shell,$2/debian/$1/usr/share/doc/$1)")

#
# $(call install-doc-files,<package>,<deb-work-dir>,<doc-file>*)
#
install-doc-files = $(foreach f,$3,$(newline)$(tab)$(usp-cp) "$(call to-shell,$f)" "$(call to-shell,$2/debian/$1/usr/share/doc/$1/$(notdir $f))")

#
# $(call make-service-dir,<package>,<deb-work-dir>,<service-file>*)
#
make-service-dir = $(if $(strip $3),$(newline)$(tab)$(usp-mkdir-p) "$(call to-shell,$2/debian/$1/lib/systemd/system)")

#
# $(call install-services,<package>,<deb-work-dir>,<service-file>*)
#
install-services = $(foreach f,$3,$(newline)$(tab)$(usp-cp) "$(call to-shell,$f)" "$(call to-shell,$2/debian/$1/lib/systemd/system/$(call service-name,$f)$(call service-suffix,$f))")

#
# $(call make-apache-conf-dir,<package>,<deb-work-dir>,<apache-conf-file>*)
#
make-apache-conf-dir = $(if $(strip $3),$(newline)$(tab)$(usp-mkdir-p) "$(call to-shell,$2/debian/$1/$(patsubst /%,%,$(apache-conf-dir))/mods-available)")

#
# $(call install-apache-conf-files,<package>,<deb-work-dir>,<apache-conf-file>*)
#
install-apache-conf-files = $(foreach f,$3,$(newline)$(tab)$(call subst-or-copy-apache-conf,$f,$2/debian/$1/$(patsubst /%,%,$(apache-conf-dir))/mods-available))

#
# $(call rules-content,<package name>,<package version>,<package revision>,<deb work dir>,<artifacts-dir>,<artifact>*,<conf-file>*,<doc-file>*,<service-file>*,<apache-conf-or-load-file>*)
#
define rules-content =
#!$(call get-make) -f

%:
	dh $$@

override_dh_auto_clean:
override_dh_auto_test:
override_dh_auto_build:
override_dh_installchangelogs:
override_dh_compress:
$(if $(with-symbol-pkg),,override_dh_strip:)
override_dh_strip_nondeterminism:

override_dh_auto_install: $(call make-artifact-dirs,$1,$4,$6)$(call install-artifacts,$1,$4,$5,$6)$(call make-conf-dir,$1,$4,$7)$(call install-conf-files,$1,$4,$7)$(call make-doc-dir,$1,$4,$8)$(call install-doc-files,$1,$4,$8)$(call make-service-dir,$1,$4,$9)$(call install-services,$1,$4,$9)$(call make-apache-conf-dir,$1,$4,$(10))$(call install-apache-conf-files,$1,$4,$(10))

override_dh_shlibdeps:
	dh_shlibdeps --dpkg-shlibdeps-params=--ignore-missing-info

override_dh_gencontrol:
	dh_gencontrol -- -v$2-$3

endef

#
# Set required debhelper version
#
override debhelper-version := 11

#
# Get system-provided settings
#
override deb-arch := $(call get-deb-arch)

#
# Check that the package is not installed, as that may interfere with
# automatic shared library dependency detection from depending packages
#
$(call check-package-not-installed,$(package))

#
# Set some derived variables
#
override ubuntu := $(strip \
  $(if $(filter %ubuntu,$(shell . /etc/os-release && echo $$ID)), \
    yes \
  ) \
)

override deb-package-basename := $(package)_$(pkg-version)-$(pkg-revision)_$(deb-arch)

override deb-package-filename := $(deb-package-basename).deb

override ddeb-package-filename := $(package)-dbgsym_$(pkg-version)-$(pkg-revision)_$(deb-arch)$(if $(ubuntu),.ddeb,.deb)

override deb-work-dir := $(packaging-work-dir)/$(deb-package-basename)

override artifacts := $(patsubst $(artifacts-dir)/%,%,$(call find-files,%,$(artifacts-dir)))

#
# Rules
#
.PHONY: all
all: deb-package

.PHONY: deb-package
deb-package: $(pkgs-dir)/$(deb-package-filename)

$(pkgs-dir)/$(deb-package-filename): \
  $(packaging-work-dir)/$(deb-package-filename) \
  | $(pkgs-dir)
ifdef with-symbol-pkg
	$(usp-cp) "$(call to-shell,$(packaging-work-dir)/$(ddeb-package-filename))" "$(call to-shell,$(pkgs-dir)/$(ddeb-package-filename))"
endif
	$(usp-cp) "$(call to-shell,$(packaging-work-dir)/$(deb-package-filename))" "$(call to-shell,$(pkgs-dir)/$(deb-package-filename))"

$(packaging-work-dir)/$(deb-package-filename): \
  $(deb-work-dir)/debian/changelog \
  $(deb-work-dir)/debian/compat \
  $(deb-work-dir)/debian/control \
  $(deb-work-dir)/debian/rules \
  | $(packaging-work-dir)
	unset MAKEFLAGS && cd "$(call to-shell,$(deb-work-dir))" && dpkg-buildpackage -us -uc -b 

$(pkgs-dir) $(packaging-work-dir):
	$(usp-mkdir-p) "$(call to-shell,$@)"

$(deb-work-dir)/debian/changelog: $(deb-work-dir)/debian
	$(file >$@,$(call changelog-content,$(package),$(pkg-version)))
	$(info generated $@)
	
$(deb-work-dir)/debian/compat: $(deb-work-dir)/debian
	$(file >$@,$(debhelper-version))
	$(info generated $@)

$(deb-work-dir)/debian/control: $(deb-work-dir)/debian
	$(file >$@,$(call control-content,$(package),$(pkg-maintainer),$(pkg-description),$(pkg-version),$(pkg-revision),$(foreach p,$(prereq-packages),$p)))
	$(info generated $@)

$(deb-work-dir)/debian/rules: $(deb-work-dir)/debian
	$(file >$@,$(call rules-content,$(package),$(pkg-version),$(pkg-revision),$(deb-work-dir),$(artifacts-dir),$(artifacts),$(conf-files),$(doc-files),$(service-files),$(apache-conf-files) $(apache-load-files)))
	$(info generated $@)
	chmod +x "$(call to-shell,$@)"

$(deb-work-dir)/debian: clean-deb-work-dir
	$(usp-mkdir-p) "$(call to-shell,$@)"

.PHONY: clean-deb-work-dir
clean-deb-work-dir:
	$(usp-rm-dir) "$(call to-shell,$(deb-work-dir))"
	$(usp-mkdir-p) "$(call to-shell,$(deb-work-dir))"
