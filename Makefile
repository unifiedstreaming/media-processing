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

include include/USPCommon.mki

#
# Supported user targets
# 
.PHONY: all
all: x264_encoding_service

.PHONY: install
install: install_x264_encoding_service

.PHONY: unit_tests
unit_tests: cuti_unit_tests x264_es_utils_unit_tests

.PHONY: clean
clean:
	$(rmdir) "$(call to-shell,$(build-dir))"

#
# Dependencies
#
.PHONY: x264_encoding_service
x264_encoding_service: x264_es_utils cuti
	$(bjam) $(call bjam_args,x264_encoding_service/.bjam)

.PHONY: install_x264_encoding_service
install_x264_encoding_service: x264_encoding_service
	$(bjam) $(call bjam_args,x264_encoding_service/install.bjam)	

.PHONY: x264_es_utils
x264_es_utils: cuti x264_proto x264
	$(bjam) $(call bjam_args,x264_es_utils/x264_es_utils/.bjam)

.PHONY: x264_es_utils_unit_tests
x264_es_utils_unit_tests: x264_es_utils
	$(bjam) $(call bjam_args,x264_es_utils/unit_tests/.bjam)

.PHONY: x264_proto
x264_proto: cuti
	$(bjam) $(call bjam_args,x264_proto/x264_proto/.bjam)
	
.PHONY: cuti
cuti:
	$(bjam) $(call bjam_args,cuti/cuti/.bjam)

.PHONY: cuti_unit_tests
cuti_unit_tests: cuti
	$(bjam) $(call bjam_args,cuti/unit_tests/.bjam)

.PHONY: x264
x264:
	$(MAKE) -C x264 -f USPMakefile $(build-settings) stage
