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

# Determine required directories
required-dirs := $(dst-dir) $(addprefix $(dst-dir)/,$(dirs-in-src))

# Determine files to keep up to date
updated-files := $(addprefix $(dst-dir)/,$(files-in-src))
  
.PHONY: all
all: $(required-dirs) $(updated-files)

$(required-dirs):
	$(usp-mkdir-p) "$(call to-shell,$@)"

$(updated-files): | $(required-dirs)
$(updated-files): $(dst-dir)/%: $(src-dir)/%
	$(usp-cp) "$(call to-shell,$<)" "$(call to-shell,$@)"
