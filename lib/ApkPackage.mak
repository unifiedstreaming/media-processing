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
override apk-package-basename := $(package)$(build-settings-suffix)-$(pkg-version)-r$(pkg-revision)
override apk-work-dir := $(packaging-work-dir)/apk/$(apk-package-basename)

override artifacts := $(patsubst $(artifacts-dir)/$(package)/%,%,$(call find-files,%,$(artifacts-dir)/$(package)))

#
# $ (call fake-git-content)
#
define fake-git-content :=
#! /bin/sh
exit 0

endef

#
# $(call post-install-content,<service name>*)
#
define post-install-content =
#!/bin/sh
$(foreach s,$1,$(newline)rc-update add $s default)

exit 0

endef

#
# $(call pre-deinstall-content,<service name>*)
#
define pre-deinstall-content =
#!/bin/sh
$(foreach s,$(call reverse,$1),$(newline)rc-service --ifstarted $s stop)
$(foreach s,$(call reverse,$1),$(newline)rc-update delete $s default)

exit 0

endef

#
# $(call post-upgrade-content,<service name>*)
#
define post-upgrade-content =
#!/bin/sh
$(foreach s,$1,$(newline)rc-service --ifstarted $s restart)

exit 0

endef

#
# $(call apkbuild-content,<package>,<build settings suffix>,<version>,<revision>,<description>,<maintainer>,<prereq package>*,<license>,<artifacts-dir>,<artifact>*,<openrc file template>*)
#
define apkbuild-content =
pkgname="$1$2"
pkgver="$3"
pkgrel="$4"
pkgdesc="$5"
# maintainer="$6" (abuild requires a valid RFC822 address)
url="FIXME"
arch="all"
license="$8"
depends="$(foreach p,$7,$p$2=$3-r$4)"
subpackages=""
source=""
options="!fhs !strip"
$(if $(strip $(11)),install="$1$2.post-install $1$2.pre-deinstall $1$2.post-upgrade")

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
  $(foreach d,$(call distro-dirs,$(10)),$(usp-mkdir-p) "$$pkgdir/$(call to-shell,$d)"$(newline)$(space))
  $(foreach a,$(10),$(usp-cp) "$(call to-shell,$9/$a)" "$$pkgdir/$(call to-shell,$(call distro-path,$a))"$(newline)$(space))
  $(if $(strip $(11)),$(usp-mkdir-p) "$$pkgdir/etc/init.d"$(newline))
  $(foreach t,$(11),$(usp-sed) 's/@BSS@/$2/g' "$(call to-shell,$t)" >"$$pkgdir/etc/init.d/$(call to-shell,$(call service-name,$t))"$(newline)$(space))
  $(foreach t,$(11),chmod +x "$(call to-shell,$t)" "$$pkgdir/etc/init.d/$(call to-shell,$(call service-name,$t))"$(newline)$(space))

  return 0
}

endef

#
# Ensure our fake git command is found first
#
export PATH := $(apk-work-dir)/fake-git:$(PATH)

.PHONY: all
all: apk-package

.PHONY: apk-package
apk-package: $(apk-work-dir)/APKBUILD \
  $(apk-work-dir)/fake-git/git \
  $(if $(strip $(openrc-file-templates)),$(addprefix $(apk-work-dir)/$(package)$(build-settings-suffix),.post-install .pre-deinstall .post-upgrade)) \
  | $(pkgs-dir)/apk/$(apk-arch)
	cd "$(call to-shell,$(apk-work-dir))" && abuild -m -P "$(call to-shell,$(pkgs-dir))" cleanpkg
	cd "$(call to-shell,$(apk-work-dir))" && abuild -m -d -P "$(call to-shell,$(pkgs-dir))"

$(apk-work-dir)/APKBUILD: clean-apk-work-dir
	$(file >$@,$(call apkbuild-content,$(package),$(build-settings-suffix),$(pkg-version),$(pkg-revision),$(pkg-description),$(pkg-maintainer),$(prereq-packages),$(license),$(artifacts-dir)/$(package),$(artifacts),$(openrc-file-templates)))
	$(info generated $@)
	
#
# install a fake git command in $(apk-work-dir)/fake-git
# (to stop abuild's git magic)
#
$(apk-work-dir)/fake-git/git: $(apk-work-dir)/fake-git
	$(file >$@,$(fake-git-content))
	$(info generated fake $@)
	chmod +x "$(call to-shell,$@)"
	
$(apk-work-dir)/fake-git: clean-apk-work-dir
	$(usp-mkdir-p) "$(call to-shell,$@)"

$(apk-work-dir)/$(package)$(build-settings-suffix).post-install: clean-apk-work-dir
	$(file >$@,$(call post-install-content,$(foreach t,$(service-file-templates),$(call service-name,$t))))
	$(info generated $@)
	chmod +x "$(call to-shell,$@)"
	
$(apk-work-dir)/$(package)$(build-settings-suffix).pre-deinstall: clean-apk-work-dir
	$(file >$@,$(call pre-deinstall-content,$(foreach t,$(service-file-templates),$(call service-name,$t))))
	$(info generated $@)
	chmod +x "$(call to-shell,$@)"
	
$(apk-work-dir)/$(package)$(build-settings-suffix).post-upgrade: clean-apk-work-dir
	$(file >$@,$(call post-upgrade-content,$(foreach t,$(service-file-templates),$(call service-name,$t))))
	$(info generated $@)
	chmod +x "$(call to-shell,$@)"
	
$(pkgs-dir)/apk/$(apk-arch):
	$(usp-mkdir-p) "$(call to-shell,$@)"	
	
.PHONY: clean-apk-work-dir
clean-apk-work-dir:
	$(usp-rm-rf) "$(call to-shell,$(apk-work-dir))"
	$(usp-mkdir-p) "$(call to-shell,$(apk-work-dir))"
