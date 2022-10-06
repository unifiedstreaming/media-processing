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

#
# This makefile updates a jamfile in the stage directory
#

include USPCommon.mki

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
