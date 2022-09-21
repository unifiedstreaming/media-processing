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
# Based on Vincent Bernat's 'Pragmatic Debian packaging'
#
# https://vincent.bernat.ch/en/blog/2019-pragmatic-debian-packaging
#
# Thanks Vincent!
#

include include/USPPackaging.mki

#
# $(call is-installed,<package>)
#
is-installed = $(shell dpkg -l "$1" >/dev/null 2>&1 && echo yes)

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
# $(call service-name,<service file template>)
#
service-name = $(basename $(notdir $1))$(build-settings-suffix)

#
# $(call make-artifact-dirs,<package>,<deb-work-dir>,<artifact>*)
#
make-artifact-dirs = $(foreach d,$(call distro-dirs,$3),$(newline)$(tab)$(usp-mkdir-p) "$(call to-shell,$2/debian/$1/$d)")

#
# $(call install-artifacts,<package>,<deb-work-dir>,<artifacts-dir>,<artifact>*)
#
install-artifacts = $(foreach a,$4,$(newline)$(tab)$(usp-cp) "$(call to-shell,$3/$a)" "$(call to-shell,$2/debian/$1/$(call distro-path,$a))")

#
# $(call make-service-dir,<package>,<deb-work-dir>,<service-file-template>*)
#
make-service-dir = $(if $(strip $3),$(newline)$(tab)$(usp-mkdir-p) "$(call to-shell,$2/debian/$1/lib/systemd/system)")

#
# $(call install-services,<package>,<deb-work-dir>,<service-file-template>*)
#
install-services = $(foreach t,$3,$(newline)$(tab)$(usp-sed) 's/@BSS@/$(build-settings-suffix)/g' "$(call to-shell,$t)" >"$(call to-shell,$2/debian/$1/lib/systemd/system/$(call service-name,$t).service)")

#
# $(call rules-content,<package name>,<package version>,<package revision>,<deb work dir>,<artifacts-dir>,<artifact>*,<service-file-template>*)
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
override_dh_strip:
override_dh_strip_nondeterminism:

override_dh_auto_install: $(call make-artifact-dirs,$1,$4,$6)$(call install-artifacts,$1,$4,$5,$6)$(call make-service-dir,$1,$4,$7)$(call install-services,$1,$4,$7)

override_dh_gencontrol:
	dh_gencontrol -- -v$2-$3

endef

#
# $(call conffiles-content,<conf-artifact>*)
#
conffiles-content = $(foreach a,$1,/$(call distro-path,$a)$(newline))

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
$(call check-package-not-installed,$(package)$(build-settings-suffix))

#
# Set some derived variables
#
override deb-package-basename := $(package)$(build-settings-suffix)_$(pkg-version)-$(pkg-revision)_$(deb-arch)

override deb-work-dir := $(packaging-work-dir)/deb/$(deb-package-basename)

override artifacts := $(patsubst $(artifacts-dir)/$(package)/%,%,$(call find-files,%,$(artifacts-dir)/$(package)))

#
# Rules
#
.PHONY: all
all: deb-package

.PHONY: deb-package
deb-package: $(pkgs-dir)/$(deb-package-basename).deb

$(pkgs-dir)/$(deb-package-basename).deb: \
  $(packaging-work-dir)/deb/$(deb-package-basename).deb \
  | $(pkgs-dir)
	$(usp-cp) "$(call to-shell,$<)" "$(call to-shell,$@)"

$(packaging-work-dir)/deb/$(deb-package-basename).deb: \
  $(deb-work-dir)/debian/changelog \
  $(deb-work-dir)/debian/compat \
  $(deb-work-dir)/debian/control \
  $(deb-work-dir)/debian/rules \
  | $(packaging-work-dir)/deb
	unset MAKEFLAGS && cd "$(call to-shell,$(deb-work-dir))" && dpkg-buildpackage -us -uc -b 

$(pkgs-dir) $(packaging-work-dir)/deb:
	$(usp-mkdir-p) "$(call to-shell,$@)"

$(deb-work-dir)/debian/changelog: $(deb-work-dir)/debian
	$(file >$@,$(call changelog-content,$(package)$(build-settings-suffix),$(pkg-version)))
	$(info generated $@)
	
$(deb-work-dir)/debian/compat: $(deb-work-dir)/debian
	$(file >$@,$(debhelper-version))
	$(info generated $@)

$(deb-work-dir)/debian/control: $(deb-work-dir)/debian
	$(file >$@,$(call control-content,$(package)$(build-settings-suffix),$(pkg-maintainer),$(pkg-description),$(pkg-version),$(pkg-revision),$(foreach p,$(prereq-packages),$p$(build-settings-suffix))))
	$(info generated $@)

$(deb-work-dir)/debian/rules: $(deb-work-dir)/debian
	$(file >$@,$(call rules-content,$(package)$(build-settings-suffix),$(pkg-version),$(pkg-revision),$(deb-work-dir),$(artifacts-dir)/$(package),$(artifacts),$(service-file-templates)))
	$(info generated $@)
	chmod +x "$(call to-shell,$@)"

$(deb-work-dir)/debian: clean-deb-work-dir
	$(usp-mkdir-p) "$(call to-shell,$@)"

.PHONY: clean-deb-work-dir
clean-deb-work-dir:
	$(usp-rm-rf) "$(call to-shell,$(deb-work-dir))"
	$(usp-mkdir-p) "$(call to-shell,$(deb-work-dir))"
