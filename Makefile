#
# MIT License: https://mit-license.org/
#
# Copyright © 2022 Varanda Labs Inc.
#

LVGL_LIB = ext/liblvgl.a

$(LVGL_LIB):
	echo "******************************************"
	echo "*                                        *"
	echo "*            Build LVGL library          *"
	echo "*                                        *"
	echo "******************************************"
	make -C ./ext

all: $(LVGL_LIB)

