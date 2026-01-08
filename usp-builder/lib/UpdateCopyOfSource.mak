#
# Copyright (C) 2023-2026 CodeShop B.V.
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
# Update (or create) a copy of some source tree $(src-dir) placed
# under $(work-dir), for further augmentation by some other tool.
#
# This utility keeps a state file to record which files and
# directories it previously placed in the copy; it does not touch any
# other files or directories in the copy.
#

include usp-builder/USPCommon.mki

MAKEFLAGS += --no-builtin-rules

# Check required parameters
override src-dir := $(abspath $(call required-value,src-dir))
override work-dir := $(abspath $(call required-value,work-dir))

# Check $(src-dir) exists
$(if $(wildcard $(src-dir)/.),,$(error directory '$(src-dir)' not found))

# Forbid overlapping src and work dirs
$(call check-out-of-tree,$(src-dir),$(work-dir))
$(call check-out-of-tree,$(work-dir),$(src-dir))

# Determine the destination directory and state file names
override package-name := $(notdir $(src-dir))
override dst-dir := $(work-dir)/$(package-name)
override state-file := $(work-dir)/$(package-name)-state.mki

# Determine current file and directory names
override curr-filenames := $(patsubst $(src-dir)/%,%, \
  $(call find-files,$(src-dir)/%,$(src-dir)))
override curr-dirnames := $(patsubst $(src-dir)/%,%, \
  $(call find-dirs,$(src-dir)/%,$(src-dir)))

# Get previously updated file and directory names by
# reading the state file (if it exists)
override prev-filenames :=
override prev-dirnames :=
-include $(state-file)

# Determine which files and directories to remove
override removed-files := $(addprefix $(dst-dir)/, \
  $(filter-out $(curr-filenames) $(curr-dirnames),$(prev-filenames)))
override removed-dirs := $(addprefix $(dst-dir)/, \
  $(filter-out $(curr-filenames) $(curr-dirnames),$(prev-dirnames)))

# Determine which directories to change into files, and which files
# to change into directories
override from-dir-files := $(addprefix $(dst-dir)/, \
  $(filter $(prev-dirnames),$(curr-filenames)))
override from-file-dirs := $(addprefix $(dst-dir)/, \
  $(filter $(prev-filenames),$(curr-dirnames)))

# Determine which other file and directoy names to update
override other-files := $(addprefix $(dst-dir)/, \
  $(filter-out $(prev-dirnames),$(curr-filenames)))
override other-dirs :=  $(addprefix $(dst-dir)/, \
  $(filter-out $(prev-filenames),$(curr-dirnames)))

.PHONY: all
all: $(removed-files) $(removed-dirs) \
  $(from-dir-files) $(from-file-dirs) \
  $(other-files) $(other-dirs) \
  | $(work-dir) $(dst-dir)
	$(file >$(state-file),override prev-filenames := $(curr-filenames)$(newline)override prev-dirnames := $(curr-dirnames))
	@echo updated $(state-file)

.PHONY: $(removed-files)
$(removed-files):
	$(usp-rm-file) "$(call to-shell,$@)"

.PHONY: $(removed-dirs)
$(removed-dirs): $(removed-files)
	$(usp-rm-dir) "$(call to-shell,$@)"

.PHONY: $(from-dir-files)
$(from-dir-files): | $(from-file-dirs) $(other-dirs) $(dst-dir)
$(from-dir-files): $(dst-dir)/%: $(src-dir)/%
	$(usp-rm-dir) "$(call to-shell,$@)"
	$(usp-cp) "$(call to-shell,$<)" "$(call to-shell,$@)"

.PHONY: $(from-file-dirs)
$(from-file-dirs): | $(dst-dir)
	$(usp-rm-file) "$(call to-shell,$@)"
	$(usp-mkdir-p) "$(call to-shell,$@)"
	
$(other-files): | $(from-file-dirs) $(other-dirs) $(dst-dir)
$(other-files): $(dst-dir)/%: $(src-dir)/%
	$(usp-cp) "$(call to-shell,$<)" "$(call to-shell,$@)"

$(other-dirs): | $(dst-dir)
	$(usp-mkdir-p) "$(call to-shell,$@)"
	
$(dst-dir): | $(work-dir)

$(dst-dir) $(work-dir):
	$(usp-mkdir-p) "$(call to-shell,$@)"
