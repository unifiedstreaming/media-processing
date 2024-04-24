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

.NOTPARALLEL:

goals-except-all := $(filter-out all,$(MAKECMDGOALS))

.PHONY: all $(goals-except-all)
all $(goals-except-all):
	$(MAKE) -I usp-builder/include -f usp-builder/lib/Main.mak "$@"

