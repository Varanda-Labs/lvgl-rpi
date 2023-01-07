#
# MIT License: https://mit-license.org/
#
# Copyright © 2022 Varanda Labs Inc.
#

APP_NAME = lvgl_app
LVGL_LIB = ext/liblvgl.a

.PHONY: all
all: $(APP_NAME)


$(LVGL_LIB):
	echo "******************************************"
	echo "*                                        *"
	echo "*            Build LVGL library          *"
	echo "*                                        *"
	echo "******************************************"
	make -C ./ext

#	app/GuitarPedals/src/ui_props_generic.c

SOURCES = \
	lvgl-port/integration.c \
	lvgl-port/log.c \
	app/GuitarPedals/src/ui_img_number_9_png.c \
	app/GuitarPedals/src/logo_images/ui_img_0021_png.c \
	app/GuitarPedals/src/logo_images/logo_screen.c \
	app/GuitarPedals/src/logo_images/ui_img_0016_png.c \
	app/GuitarPedals/src/logo_images/ui_img_0020_png.c \
	app/GuitarPedals/src/logo_images/ui_img_about_png.c \
	app/GuitarPedals/src/logo_images/ui_img_0007_png.c \
	app/GuitarPedals/src/logo_images/ui_img_0006_png.c \
	app/GuitarPedals/src/logo_images/ui_img_0023_png.c \
	app/GuitarPedals/src/logo_images/ui_img_0011_png.c \
	app/GuitarPedals/src/logo_images/ui_img_0002_png.c \
	app/GuitarPedals/src/logo_images/ui_img_0019_png.c \
	app/GuitarPedals/src/logo_images/ui_img_0024_png.c \
	app/GuitarPedals/src/logo_images/ui_img_0014_png.c \
	app/GuitarPedals/src/logo_images/ui_img_0003_png.c \
	app/GuitarPedals/src/logo_images/ui_img_0022_png.c \
	app/GuitarPedals/src/logo_images/ui_img_0013_png.c \
	app/GuitarPedals/src/logo_images/ui_img_0015_png.c \
	app/GuitarPedals/src/logo_images/ui_img_0010_png.c \
	app/GuitarPedals/src/logo_images/ui_img_0018_png.c \
	app/GuitarPedals/src/logo_images/ui_img_0004_png.c \
	app/GuitarPedals/src/logo_images/ui_img_0001_png.c \
	app/GuitarPedals/src/logo_images/ui_img_0000_png.c \
	app/GuitarPedals/src/logo_images/ui_img_0009_png.c \
	app/GuitarPedals/src/logo_images/ui_img_0012_png.c \
	app/GuitarPedals/src/logo_images/ui_img_0017_png.c \
	app/GuitarPedals/src/logo_images/ui_img_0005_png.c \
	app/GuitarPedals/src/logo_images/ui_img_0008_png.c \
	app/GuitarPedals/src/ui_img_number_4_png.c \
	app/GuitarPedals/src/ui_img_left_arrow_png.c \
	app/GuitarPedals/src/ui_img_sel_props_png.c \
	app/GuitarPedals/src/ui_img_number_3_png.c \
	app/GuitarPedals/src/ui_img_pedal_volume_png.c \
	app/GuitarPedals/src/ui_img_sel_check_png.c \
	app/GuitarPedals/src/ui_img_disp_background_png.c \
	app/GuitarPedals/src/ui_img_number_7_png.c \
	app/GuitarPedals/src/ui_img_sel_enable_png.c \
	app/GuitarPedals/src/ui_img_sel_move_left_png.c \
	app/GuitarPedals/src/ui_img_sel_close_png.c \
	app/GuitarPedals/src/ui_img_screen_icon_png.c \
	app/GuitarPedals/src/util.c \
	app/GuitarPedals/src/pedal_volume.c \
	app/GuitarPedals/src/ui_img_number_1_png.c \
	app/GuitarPedals/src/ui_img_number_2_png.c \
	app/GuitarPedals/src/pedal_compressor.c \
	app/GuitarPedals/src/ui_img_arrows_down_png.c \
	app/GuitarPedals/src/ui_img_arrows_up_png.c \
	app/GuitarPedals/src/ui_helpers.c \
	app/GuitarPedals/src/ui_img_right_arrow_png.c \
	app/GuitarPedals/src/ui_img_sel_move_right_png.c \
	app/GuitarPedals/src/ui_img_pedal_dist_png.c \
	app/GuitarPedals/src/ui_img_board_label_png.c \
	app/GuitarPedals/src/pedal_fuzz.c \
	app/GuitarPedals/src/ui_img_boards_icon_png.c \
	app/GuitarPedals/src/ui_img_play_icon_png.c \
	app/GuitarPedals/src/ui_img_guitar_background_01_png.c \
	app/GuitarPedals/src/ui_img_pedal_fuzz_png.c \
	app/GuitarPedals/src/ui_img_padels_label_png.c \
	app/GuitarPedals/src/generic_props.c \
	app/GuitarPedals/src/visual_audio.c \
	app/GuitarPedals/src/ui_img_sel_remove_png.c \
	app/GuitarPedals/src/ui_img_stop_icon_png.c \
	app/GuitarPedals/src/ui_img_number_0_png.c \
	app/GuitarPedals/src/ui_img_number_8_png.c \
	app/GuitarPedals/src/ui_img_pedal_compr_png.c \
	app/GuitarPedals/src/ui_img_number_5_png.c \
	app/GuitarPedals/src/ui_img_pedal_echo_png.c \
	app/GuitarPedals/src/ui_img_pedal_empty_png.c \
	app/GuitarPedals/src/ui_img_number_6_png.c \
	app/GuitarPedals/src/pedal_echo.c \
	app/GuitarPedals/src/pedal_distortion.c \
	app/GuitarPedals/src/ui.c \
	app/GuitarPedals/guitar_pedals_main.c \


OBJS := ${SOURCES:.c=.o}

CFLAGS += -g -O0 -DDEBUG -fmax-errors=3
CFLAGS += -DGUITAR_PEDALS

INCLUDEPATH = \
 	-I . \
 	-I lvgl-port \
	-I app/GuitarPedals \
	-I app/GuitarPedals/src \
	-I app/GuitarPedals/src/logo_images \
 	-I ext/lvgl \
	-I ext/lvgl/src/hal \
	-I ext/lvgl/src/misc \

CFLAGS += ${INCLUDEPATH}
CFLAGS +=  -DUSE_APP_LOG -DLV_CONF_INCLUDE_SIMPLE

LDFLAGS += -L . -L /ext

C=gcc

%.o: %.c
	$(C) $(CFLAGS) -fPIC $(INCLUDE) -c $< -o $@

$(APP_NAME): $(LVGL_LIB) $(OBJS) 
	g++ $(CFLAGS) $(OBJS) -l:$(LVGL_LIB) $(LDFLAGS) -l pthread -o $@

clean:
	make -C ext clean
	find . -type f -name '*.o' -delete



