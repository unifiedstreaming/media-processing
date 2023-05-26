#
# Copyright (C) 2023 CodeShop B.V.
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

include usp-builder/USPCommon.mki

# Check required parameters
override src-dir := $(call required-value,src-dir)
override dst-dir := $(call required-value,dst-dir)

# Optional parameter: $(exclude) excluded files and dirs

# Check that src-dir exists
$(if $(call is-dir,$(src-dir)),,$(error directory '$(src-dir)' not found)) 

# Forbid overlapping src and dst
$(call check-out-of-tree,$(src-dir),$(dst-dir))
$(call check-out-of-tree,$(dst-dir),$(src-dir))

# We first purge $(dst-dir) by removing files and directories not
# found in $(src-dir). 
# Then, we update the files in $(dst-dir)
.PHONY: all
all:
	$(MAKE) -f $(usp-builder-lib-dir)/PurgeDstDir.mak \
	  src-dir="$(src-dir)" dst-dir="$(dst-dir)" exclude="$(exclude)"
	$(MAKE) -f $(usp-builder-lib-dir)/UpdateDstDir.mak \
	  src-dir="$(src-dir)" dst-dir="$(dst-dir)" exclude="$(exclude)"


