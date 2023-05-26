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
override exclude := $(foreach e,$(exclude),$e $e/%)

# Require that src-dir exists
$(if $(call is-dir,$(src-dir)),,$(error directory '$(src-dir)' not found)) 

# Forbid overlapping src and dst dirs
$(call check-out-of-tree,$(src-dir),$(dst-dir))
$(call check-out-of-tree,$(dst-dir),$(src-dir))

# Determine relative paths to files and directories under $(src_dir)
files-in-src := $(filter-out $(exclude), \
  $(patsubst $(src-dir)/%,%,$(call find-files,%,$(src-dir)/*)) \
)
dirs-in-src := $(filter-out $(exclude), \
  $(patsubst $(src-dir)/%,%,$(call find-dirs,%,$(src-dir)/*)) \
)
  
# Determine relative paths to files and directories under $(dst_dir)
files-in-dst := $(filter-out $(exclude), \
  $(patsubst $(dst-dir)/%,%,$(call find-files,%,$(dst-dir)/*)) \
)
dirs-in-dst := $(filter-out $(exclude), \
  $(patsubst $(dst-dir)/%,%,$(call find-dirs,%,$(dst-dir)/*)) \
)
  
# Determine which directory entries in $(dst-dir) to purge. Please note
# that this includes directories that are files in $(src-dir) and
# files that are directories in $(src-dir).
purged-entries := $(addprefix $(dst-dir)/, \
  $(filter-out $(files-in-src),$(files-in-dst)) \
  $(filter-out $(dirs-in-src),$(dirs-in-dst)) \
)

.PHONY: all
all: $(purged-entries)

.PHONY: $(purged-entries)
$(purged-entries):
	$(usp-rm-rf) "$(call to-shell,$@)"
