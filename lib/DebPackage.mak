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
# $(call checked-deb-arch-output,<output>)
#
checked-dpkg-architecture-output = $(strip \
  $(if $(filter-out 1,$(words $1)), \
    $(error dpkg failed to report architecture) \
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
# $(call is-elf-file-output,<output>)
#
is-elf-file-output = $(strip \
  $(if $(filter 0,$(words $1)), \
    $(error 'file' command failure) \
  ) \
  $(if $(filter ELF,$1), \
    yes \
  ) \
)

#
# $(call is-elf-file,<file>)
#
is-elf-file = $(call is-elf-file-output,$(shell file "$(call to-shell,$1)"))

#
# $(call find-elf-files,<directory>)
# Returns directory-relative paths
#
find-elf-files = $(strip \
  $(foreach f,$(call find-files,%,$1), \
    $(if $(call is-elf-file,$f), \
      $(patsubst $1/%,%,$f) \
    ) \
  ) \
)

#
# $(call dpkg-shlibdeps-list,<remaining word>*,<buffered word>*)
#
dpkg-shlibdeps-list = $(strip \
  $(if $(filter 0,$(words $1)), \
    $(subst $(space),,$2) \
  , $(if $(filter $(comma),$(firstword $1)), \
      $(subst $(space),,$2) \
      $(call dpkg-shlibdeps-list,$(wordlist 2,$(words $1),$1)) \
    , \
      $(call dpkg-shlibdeps-list,$(wordlist 2,$(words $1),$1), \
        $2 $(firstword $1)) \
    ) \
  ) \
)

#
# $(call checked-dpkg-shlibdeps-list,<word>*)
#
checked-dpkg-shlibdeps-list = $(strip \
  $(if $(filter 0,$(words $1)), \
    $(error dpkg-shlibdeps failure) \
  ) \
  $1 \
)

#
# $(call system-shlibdeps-impl,<directory>,<elf file>+)
#
system-shlib-deps-impl = $(strip \
  $(if $(firstword $2), \
    $(call checked-dpkg-shlibdeps-list, \
      $(call dpkg-shlibdeps-list, \
        $(subst $(comma),$(space)$(comma)$(space), \
          $(subst shlibs:Depends=,, \
            $(shell \
              cd "$(call to-shell,$1)" && \
              dpkg-shlibdeps -O $(foreach f,$2,"$(call to-shell,$f)" \
                2>/dev/null) \
            ) \
          ) \
        ) \
      ) \
    ) \
  ) \
)

#
# $(call system-shlib-deps,<directory>)
#
system-shlib-deps = $(call system-shlib-deps-impl,$1,$(call find-elf-files,$1))

#
# $(call prereq-deps,<prereq package>*,<version>)
#
prereq-deps = $(strip \
  $(foreach pre,$1, \
    $(pre)$(open-paren)=$2$(close-paren) \
  ) \
)
  
#
# $(call package-deps,<prereq-package>*,<version>,<package work dir>)
#
package-deps = $(strip \
  $(call prereq-deps,$1,$2) \
  $(call system-shlib-deps,$3) \
)

#
# $(call depends-line-impl,<dep*>)
#
depends-line-impl = $(if $(strip $1),Depends: $(subst $(space),$(comma)$(space),$(strip $1))$(newline))

#
# $(call depends-line,<prereq-package>*,<build-settings-suffix>,<version>,<package-work-dir)
#
depends-line = $(call depends-line-impl,$(call package-deps,$(addsuffix $2,$1),$3,$4))

#
# $(call control-file-content,<package>,<build-settings-suffix>,<version>,<architecture>,<prereq-package>*,<package-work-dir>,<maintainer>,<description>)
#
define control-file-content =
Package: $1$2
Version: $3
Architecture: $4
$(call depends-line,$5,$2,$3,$6)Maintainer: $7
Description: $8

endef

#
# Get system-provided settings
#
override deb-arch := $(call get-deb-arch)

#
# Set some derived variables
#
override debs-work-dir := $(packaging-work-dir)/deb

override deb-package-basename := $(package)$(build-settings-suffix)_$(pkg-version)-$(pkg-revision)_$(deb-arch)

override package-work-dir := $(debs-work-dir)/$(deb-package-basename)

override artifacts := $(patsubst $(artifacts-dir)/$(package)/%,%,$(call find-files,%,$(artifacts-dir)/$(package)))

override work-dir-artifacts := $(foreach a,$(artifacts),$(package-work-dir)$(installation-prefix)/$a)

override work-dir-dirs := $(call dedup,$(filter-out $(package-work-dir),$(patsubst %/,%,$(dir $(work-dir-artifacts)))))

#
# Rules
#
.PHONY: all
all: deb-package

.PHONY: deb-package
deb-package: $(pkgs-dir)/$(deb-package-basename).deb

# Remove $(package-work-dir)/debian first to prevent it from being packaged
$(pkgs-dir)/$(deb-package-basename).deb: $(package-work-dir)/DEBIAN/control | $(pkgs-dir)
	$(usp-rm-rf) "$(call to-shell,$(package-work-dir)/debian)"
	dpkg-deb --root-owner-group --build "$(call to-shell,$(package-work-dir))" "$(call to-shell,$@)"

$(pkgs-dir):
	$(usp-mkdir-p) "$(call to-shell,$@)"

$(package-work-dir)/DEBIAN/control: $(package-work-dir)/debian/control $(package-work-dir)/DEBIAN $(work-dir-artifacts)
	$(file >$@,$(call control-file-content,$(package),$(build-settings-suffix),$(pkg-version)-$(pkg-revision),$(deb-arch),$(prereq-packages),$(package-work-dir),$(pkg-maintainer),$(pkg-description)))
	@echo generated $@

# empty file required by dpkg-shlibdeps
$(package-work-dir)/debian/control: $(package-work-dir)/debian
	$(file >$@,)
	@echo generated $@

$(package-work-dir)/debian $(package-work-dir)/DEBIAN: clean-work-dir
	$(usp-mkdir-p) "$(call to-shell,$@)"

$(work-dir-artifacts): $(work-dir-dirs)

$(work-dir-artifacts): $(package-work-dir)$(installation-prefix)/%: $(artifacts-dir)/$(package)/%
	$(usp-cp) "$(call to-shell,$<)" "$(call to-shell,$@)"

$(work-dir-dirs): clean-work-dir

$(work-dir-dirs): $(package-work-dir)$(installation-prefix)/%:
	$(usp-mkdir-p) "$(call to-shell,$@)"

.PHONY: clean-work-dir
clean-work-dir:
	$(usp-rm-rf) "$(call to-shell,$(package-work-dir))"
	$(usp-mkdir-p) "$(call to-shell,$(package-work-dir))"
