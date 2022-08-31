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
is-installed = $(shell apk -e info "$1" >/dev/null 2>&1 && echo yes)

#
# $(call check-package-not-installed,<package>)
#
check-package-not-installed = $(strip \
  $(if $(call is-installed,$1), \
    $(error package "$1" appears to be installed - please purge it first) \
  ) \
)
  
#
# $(call checked-apk-arch,<output>)
#
checked-apk-arch = $(strip \
  $(if $(filter-out 1,$(words $1)), \
    $(error 'arch' command failed) \
  ) \
  $1 \
)

#
# $(call get-apk-arch)
# TODO: there has to be a better way...
#
get-apk-arch = $(call checked-apk-arch,$(shell arch))

#
# $(call checked-sha512sum,<output>)
#
checked-sha512sum = $(strip \
  $(if $(filter-out 2,$(words $1)), \
    $(error 'sha512sum' command failed) \
  ) \
  $(firstword $1) \
)

#
# $(call sha512sum,<file>)
#
sha512sum = $(call checked-sha512sum,$(shell sha512sum "$(call to-shell,$1)"))

#
# Get system-provided settings
#
override apk-arch := $(call get-apk-arch)

#
# Check that the package is not installed, as that may interfere with
# automatic shared library dependency detection from depending packages
#
$(call check-package-not-installed,$(package)$(build-settings-suffix))

#
# Set some derived variables
#
override installation-prefix := $(call required-value,installation-prefix)

override apk-package-basename := $(package)$(build-settings-suffix)-$(pkg-version)-r$(pkg-revision)
override apk-work-dir := $(packaging-work-dir)/apk/$(apk-package-basename)

define fake-git-content :=
#! /bin/sh
exit 0

endef

#
# $(call apkbuild-content,<package>,<build settings suffix>,<version>,<revision>,<description>,<maintainer>,<prereq package>*,<source tarfile>,<installation_prefix>)
#
define apkbuild-content =
pkgname="$1$2"
pkgver="$3"
pkgrel="$4"
pkgdesc="$5"
# maintainer="$6" (abuild requires a valid RFC822 address)
url="FIXME"
arch="all"
license="FIXME"
depends="$(foreach p,$7,$p$2=$3-r$4)"
subpackages=""
source="$(call to-shell,$(notdir $8))"
builddir="$$srcdir"
options="!fhs !strip"

prepare() {
  default_prepare
}

build() {
  return 0
}

check() {
  return 0
}

package() {
  $(usp-mkdir-p) "$$pkgdir$(patsubst %/,%,$(call to-shell,$(dir $9)))"
  cp -dR "$(call to-shell,$1)" "$$pkgdir$(call to-shell,$9)"
  return 0
}

sha512sums="$(call sha512sum,$8)$(space)$(space)$(call to-shell,$(notdir $8))"

endef

#
# Ensure our fake git command is found first
#
export PATH := $(apk-work-dir)/fake-git:$(PATH)

.PHONY: all
all: apk-package

.PHONY: apk-package
apk-package: $(apk-work-dir)/APKBUILD $(apk-work-dir)/fake-git/git \
  | $(pkgs-dir)/apk/$(apk-arch)
	cd "$(call to-shell,$(apk-work-dir))" && abuild -m -P "$(call to-shell,$(pkgs-dir))" cleanpkg
	cd "$(call to-shell,$(apk-work-dir))" && abuild -m -d -P "$(call to-shell,$(pkgs-dir))"

$(apk-work-dir)/APKBUILD: $(apk-work-dir)/$(package).tar
	$(file >$@,$(call apkbuild-content,$(package),$(build-settings-suffix),$(pkg-version),$(pkg-revision),$(pkg-description),$(pkg-maintainer),$(prereq-packages),$(apk-work-dir)/$(package).tar,$(installation-prefix)))
	$(info generated $@)
	
$(apk-work-dir)/$(package).tar: clean-apk-work-dir
	cd "$(call to-shell,$(artifacts-dir))" && tar cf "$(call to-shell,$@)" "$(call to-shell,$(package))"

#
# install a fake git command in $(apk-work-dir)/fake-git
# (to stop abuild's git magic)
#
$(apk-work-dir)/fake-git/git: $(apk-work-dir)/fake-git
	$(file >$@,$(fake-git-content))
	$(info generated fake $@)
	chmod 755 "$(call to-shell,$@)"
	
$(apk-work-dir)/fake-git: clean-apk-work-dir
	$(usp-mkdir-p) "$(call to-shell,$@)"	
	
$(pkgs-dir)/apk/$(apk-arch):
	$(usp-mkdir-p) "$(call to-shell,$@)"	
	
.PHONY: clean-apk-work-dir
clean-apk-work-dir:
	$(usp-rm-rf) "$(call to-shell,$(apk-work-dir))"
	$(usp-mkdir-p) "$(call to-shell,$(apk-work-dir))"

