#
# Copyright (C) 2022-2025 CodeShop B.V.
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
# work around abuild bug: ensure the "verbose" variable is NOT exported to the
# environment of recipe commands, since abuild will then attempt to run the
# value as a command.
#
unexport verbose

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
    $(error 'abuild -A' command failed) \
  ) \
  $1 \
)

#
# $(call get-apk-arch)
#
get-apk-arch = $(call checked-apk-arch,$(shell git=true abuild -A))

#
# $(call checked-apk-package-name-impl,<package name>,<char>*)
#
# https://wiki.alpinelinux.org/wiki/Package_policies
# we allow for underscore and minus
# 
checked-apk-package-name-impl = $(strip \
  $(if $(filter-out $(lowers) $(digits),$(firstword $2)), \
    $(error $(strip \
      package name '$1' must start with a lower case letter or digit \
    )) \
  ) \
  $(if $(filter 0 1,$(words $2)), \
    $(error $(strip \
      package name '$1' must have at least two characters \
    )) \
  ) \
  $(if $(filter-out $(lowers) $(digits) - _,$(wordlist 2,$(words $2),$2)), \
    $(error $(strip \
      package name '$1' may only contain lower case letters, digits, \
        '-' and '_' \
    )) \
  ) \
  $1 \
)

#
# $(call checked-apk-package-name,<package name>)
#
checked-apk-package-name = $(strip \
  $(if $(filter-out 1,$(words $1)), \
    $(error package name '$1' must have exactly one word) \
  ) \
  $(call checked-apk-package-name-impl,$(strip $1), \
    $(call split-word,$(strip $1))) \
)

#
# $(call version-and-revision,<package>)
#
version-and-revision = $(strip \
  $(call get-package-version,$1)-r$(call get-package-revision,$1) \
)

#
# $(call depends-listing,<system package>*,<prereq package>*)
#
depends-listing = $(strip \
  $(foreach p,$1,$p) \
  $(foreach p,$2,$p=$(call version-and-revision,$p)) \
)

#
# $(call provided-sos,<apk-file-name>)
#
provided-sos = $(shell tar -x -f "$1" -O .PKGINFO | $(usp-sed) -n -e 's/^provides = so:\(.*\)=0$$/\1/p')

#
# $(call prereq-apk-path,<package-name>)
#
prereq-apk-path = $(pkgs-dir)/$1-$(call version-and-revision,$1).apk

#
# $(call provided-sos-listing,<prereq-package>*)
#
provided-sos-listing = $(strip \
  $(foreach p,$1,$(call provided-sos,$(call prereq-apk-path,$p))) \
)

override package := $(call checked-apk-package-name,$(package))

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
override abuild-output-dir := $(apk-work-dir)/abuild-output
override abuild-pkgs-dir := $(abuild-output-dir)/apk/$(apk-arch)

override artifacts := $(patsubst $(artifacts-dir)/%,%,$(call find-files-and-links,$(artifacts-dir)))

#
# Determine installation scripts
#
override post-install-script := $(if $(strip $(openrc-files) $(require-user) $(require-group)),$(package).post-install)
override post-upgrade-script := $(if $(strip $(openrc-files) $(require-user) $(require-group)),$(package).post-upgrade)
override pre-deinstall-script := $(if $(strip $(openrc-files)),$(package).pre-deinstall)
override installation-scripts := $(strip $(post-install-script) $(post-upgrade-script) $(pre-deinstall-script))

#
# $(call require-user-group-content,<user>?,<group>?)
#
define require-user-group-content =

$(if $2,if ! getent group "$2" >/dev/null 2>&1 ; then$(newline)  addgroup -S "$2"$(newline)fi)
$(if $1,if ! getent passwd "$1" >/dev/null 2>&1 ; then$(newline)  adduser -S -g "" -h / -H$(if $2, -G "$2") -s /sbin/nologin "$1"$(newline)fi)

endef

#
# $(call post-install-content,<service name>*,<require user>?,<require group>?)
#
define post-install-content =
#!/bin/sh
$(if $(strip $2 $3),$(call require-user-group-content,$2,$3))
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
# $(call post-upgrade-content,<service name>*<require user>?,<require group>?)
#
define post-upgrade-content =
#!/bin/sh
$(if $(strip $2 $3),$(call require-user-group-content,$2,$3))
$(foreach s,$1,$(newline)rc-service --ifstarted $s restart)

exit 0

endef

#
# $(call apkbuild-content,<package>,<version>,<revision>,<description>,<maintainer>,<prereq package>*,<license>,<artifacts-dir>,<artifact>*,<conf file>*,<doc file>*,<openrc file>*,<apache conf file>*,<add debug package>?,<required system package>*,<installation script>*)
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
depends="$(call depends-listing,$(15),$6)"
subpackages="$(foreach s,$(if $(14),$$pkgname-dbg),$s)"
source=""
options="!fhs$(if $(14),, !dbg !strip)"
somask="$(call provided-sos-listing,$6)"
$(if $(strip $(16)),install="$(strip $(16))")

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
  # Ensure $$pkgdir exists (even when empty)
  #
  $(usp-mkdir-p) "$(call to-shell,$$pkgdir)"
  #
  # Binaries
  #
  $(foreach d,$(call distro-dirs,$9),$(usp-mkdir-p) "$(call to-shell,$$pkgdir/$d)"$(newline)$(space))
  $(foreach a,$9,$(if $(call read-link,$8/$a),$(usp-ln-sf) "$(call to-shell,$(call read-link,$8/$a))" "$(call to-shell,$$pkgdir/$(call distro-path,$a))",$(usp-cp) "$(call to-shell,$8/$a)" "$(call to-shell,$$pkgdir/$(call distro-path,$a))")$(newline)$(space))
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
  #
  # Apache config files
  #
  $(if $(strip $(13)),$(usp-mkdir-p) "$(call to-shell,$$pkgdir/$(patsubst /%,%,$(apache-conf-dir))/conf.d)"$(newline))
  $(foreach f,$(13),$(call subst-or-copy-apache-conf,$f,$$pkgdir/$(patsubst /%,%,$(apache-conf-dir))/conf.d)$(newline)$(space))

  return 0
}

endef

.PHONY: all
all: apk-package

.PHONY: apk-package
apk-package: build-apk-packages
	$(usp-mkdir-p) "$(call to-shell,$(pkgs-dir))" $(foreach f,$(wildcard $(abuild-pkgs-dir)/*.apk),&& $(usp-cp) "$(call to-shell,$f)" "$(call to-shell,$(pkgs-dir)/$(notdir $f))")
	
.PHONY: build-apk-packages
build-apk-packages: $(apk-work-dir)/APKBUILD \
  $(addprefix $(apk-work-dir)/,$(installation-scripts))
	cd "$(call to-shell,$(apk-work-dir))" && git=true abuild -m -d -P "$(call to-shell,$(abuild-output-dir))"
	
$(apk-work-dir)/APKBUILD: clean-apk-work-dir
	$(file >$@,$(call apkbuild-content,$(package),$(pkg-version),$(pkg-revision),$(pkg-description),$(pkg-maintainer),$(prereq-packages),$(license),$(artifacts-dir),$(artifacts),$(conf-files),$(doc-files),$(openrc-files),$(apache-conf-files),$(add-debug-package),$(extra-prereq-system-packages),$(installation-scripts)))
	$(info generated $@)
	
$(addprefix $(apk-work-dir)/,$(post-install-script)): clean-apk-work-dir
	$(file >$@,$(call post-install-content,$(foreach f,$(openrc-files),$(call service-name,$f)),$(require-user),$(require-group)))
	$(info generated $@)
	chmod +x "$(call to-shell,$@)"
	
$(addprefix $(apk-work-dir)/,$(pre-deinstall-script)): clean-apk-work-dir
	$(file >$@,$(call pre-deinstall-content,$(foreach f,$(openrc-files),$(call service-name,$f))))
	$(info generated $@)
	chmod +x "$(call to-shell,$@)"
	
$(addprefix $(apk-work-dir)/,$(post-upgrade-script)): clean-apk-work-dir
	$(file >$@,$(call post-upgrade-content,$(foreach f,$(openrc-files),$(call service-name,$f)),$(require-user),$(require-group)))
	$(info generated $@)
	chmod +x "$(call to-shell,$@)"
	
.PHONY: clean-apk-work-dir
clean-apk-work-dir:
	$(usp-rm-dir) "$(call to-shell,$(apk-work-dir))"
	$(usp-mkdir-p) "$(call to-shell,$(apk-work-dir))"
