#
# Copyright (C) 2022-2023 CodeShop B.V.
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
# This makefile updates a jamfile in the stage directory
#

include usp-builder/USPCommon.mki

jamfile := $(call required-value,jamfile)
jamfile-dir := $(patsubst %/,%,$(dir $(jamfile)))

libname := $(call required-value,libname)
libfile := $(call required-value,libfile)
incdir := $(call required-value,incdir)
prereq-libs := $(strip $(prereq-libs))

.PHONY: all
all: $(jamfile)

$(jamfile): $(libfile) | $(jamfile-dir)
	$(file >$@,$(call staged-jamfile-content,$(libname),$(libfile),$(incdir),$(prereq-libs)))
	@echo updated $@

$(jamfile-dir):
	$(usp-mkdir-p) "$(call to-shell,$@)"
