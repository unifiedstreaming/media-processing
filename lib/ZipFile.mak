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
# Set some derived variables
#

override zip-file-name := $(package)$(build-settings-suffix)_$(pkg-version)-$(pkg-revision).zip

#
# Rules
#
.PHONY: all
all: zip-file

.PHONY: zip-file
zip-file: $(pkgs-dir)/$(zip-file-name)

$(pkgs-dir)/$(zip-file-name) : force | $(pkgs-dir)
	$(usp-rm-rf) "$(call to-shell,$@)"
	cd "$(call to-shell,$(artifacts-dir)/$(package))" && zip -X -r "$(call to-shell,$@)" . 

$(pkgs-dir):
	$(usp-mkdir-p) "$(call to-shell,$@)"

.PHONY: force
force:
