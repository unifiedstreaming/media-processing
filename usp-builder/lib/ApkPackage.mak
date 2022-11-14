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

include usp-builder/USPPackaging.mki

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
$(call check-package-not-installed,$(package))

#
# Set some derived variables
#
override apk-package-basename := $(package)-$(pkg-version)-r$(pkg-revision)
override apk-work-dir := $(packaging-work-dir)/$(apk-package-basename)

override artifacts := $(patsubst $(artifacts-dir)/%,%,$(call find-files,%,$(artifacts-dir)))

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
# $(call apkbuild-content,<package>,<version>,<revision>,<description>,<maintainer>,<prereq package>*,<license>,<artifacts-dir>,<artifact>*,<conf file>*,<doc file>*,<openrc file>*)
#
define apkbuild-content =
pkgname="$1"
pkgver="$2"
pkgrel="$3"
pkgdesc="$4"
# maintainer="$5" (abuild requires a valid RFC822 address)
url="FIXME"
arch="all"
license="$7"
depends="$(foreach p,$6,$p=$2-r$3)"
subpackages="$(if $(with-symbol-pkg),$$pkgname-dbg)"
source=""
options="!fhs$(if $(with-symbol-pkg),, !dbg !strip)"
$(if $(strip $(12)),install="$1.post-install $1.pre-deinstall $1.post-upgrade")

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
  #
  # Binaries
  #
  $(foreach d,$(call distro-dirs,$9),$(usp-mkdir-p) "$$pkgdir/$(call to-shell,$d)"$(newline)$(space))
  $(foreach a,$9,$(if $(call read-link,$8/$a),ln -sf "$(call to-shell,$(call read-link,$8/$a))" "$$pkgdir/$(call to-shell,$(call distro-path,$a))",$(usp-cp) "$(call to-shell,$8/$a)" "$$pkgdir/$(call to-shell,$(call distro-path,$a))")$(newline)$(space))
  #
  # Config files
  #
  $(if $(strip $(10)),$(usp-mkdir-p) "$(call to-shell,$$pkgdir/etc)"$(newline))
  $(foreach f,$(10),$(usp-cp) "$(call to-shell,$f)" "$(call to-shell,$$pkgdir/etc/$(notdir $f))"$(newline)$(space))
  #
  # Documentation files
  #
  $(if $(strip $(11)),$(usp-mkdir-p) "$(call to-shell,$$pkgdir/usr/share/doc/$1)"$(newline))
  $(foreach f,$(11),$(usp-cp) "$(call to-shell,$f)" "$(call to-shell,$$pkgdir/usr/share/doc/$1/$(notdir $f))"$(newline)$(space))
  #
  # Openrc files
  #
  $(if $(strip $(12)),$(usp-mkdir-p) "$(call to-shell,$$pkgdir/etc/init.d)"$(newline))
  $(foreach f,$(12),$(usp-cp) "$(call to-shell,$f)" "$(call to-shell,$$pkgdir/etc/init.d/$(call service-name,$f)$(call service-suffix,$f))"$(newline)$(space))
  $(foreach f,$(12),chmod +x "$(call to-shell,$$pkgdir/etc/init.d/$(call service-name,$f)$(call service-suffix,$f))"$(newline)$(space))

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
  $(if $(strip $(openrc-files)),$(addprefix $(apk-work-dir)/$(package),.post-install .pre-deinstall .post-upgrade)) \
  | $(pkgs-dir)/apk/$(apk-arch)
	cd "$(call to-shell,$(apk-work-dir))" && abuild -m -P "$(call to-shell,$(pkgs-dir))" cleanpkg
	cd "$(call to-shell,$(apk-work-dir))" && abuild -m -d -P "$(call to-shell,$(pkgs-dir))"

$(apk-work-dir)/APKBUILD: clean-apk-work-dir
	$(file >$@,$(call apkbuild-content,$(package),$(pkg-version),$(pkg-revision),$(pkg-description),$(pkg-maintainer),$(prereq-packages),$(license),$(artifacts-dir),$(artifacts),$(conf-files),$(doc-files),$(openrc-files)))
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

$(apk-work-dir)/$(package).post-install: clean-apk-work-dir
	$(file >$@,$(call post-install-content,$(foreach f,$(openrc-files),$(call service-name,$f))))
	$(info generated $@)
	chmod +x "$(call to-shell,$@)"
	
$(apk-work-dir)/$(package).pre-deinstall: clean-apk-work-dir
	$(file >$@,$(call pre-deinstall-content,$(foreach f,$(openrc-files),$(call service-name,$f))))
	$(info generated $@)
	chmod +x "$(call to-shell,$@)"
	
$(apk-work-dir)/$(package).post-upgrade: clean-apk-work-dir
	$(file >$@,$(call post-upgrade-content,$(foreach f,$(openrc-files),$(call service-name,$f))))
	$(info generated $@)
	chmod +x "$(call to-shell,$@)"
	
$(pkgs-dir)/apk/$(apk-arch):
	$(usp-mkdir-p) "$(call to-shell,$@)"	
	
.PHONY: clean-apk-work-dir
clean-apk-work-dir:
	$(usp-rm-rf) "$(call to-shell,$(apk-work-dir))"
	$(usp-mkdir-p) "$(call to-shell,$(apk-work-dir))"
