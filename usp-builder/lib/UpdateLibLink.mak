#
# Copyright (C) 2022-2024 CodeShop B.V.
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
# This makefile updates a symbolic link in $(stage-dir)/lib
#

include usp-builder/USPCommon.mki

$(if $(windows),$(error symlinks not supported))

override stage-dir := $(call required-value,stage-dir)
override work-dir := $(call required-value,work-dir)
override target-name := $(call required-value,target-name)
override link-name := $(filter-out $(target-name),$(call required-value,link-name))

override link-stamp := $(if $(link-name),$(work-dir)/$(link-name).link-stamp)

.PHONY: all
all: $(link-stamp)

$(link-stamp): $(stage-dir)/lib/$(target-name) | $(work-dir)
	$(usp-ln-sf) "$(call to-shell,$(target-name))" "$(call to-shell,$(stage-dir)/lib/$(link-name))"
	echo timestamp >"$(call to-shell,$@)"

$(work-dir):
	$(usp-mkdir-p) "$(call to-shell,$@)"
