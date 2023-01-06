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
	ext/lvgl/demos/music/lv_demo_music.c \
	ext/lvgl/demos/music/lv_demo_music_list.c \
	ext/lvgl/demos/music/lv_demo_music_main.c \
	ext/lvgl/demos/music/assets/img_lv_demo_music_btn_corner_large.c \
	ext/lvgl/demos/music/assets/img_lv_demo_music_btn_list_pause.c \
	ext/lvgl/demos/music/assets/img_lv_demo_music_btn_list_pause_large.c \
	ext/lvgl/demos/music/assets/img_lv_demo_music_btn_list_play.c \
	ext/lvgl/demos/music/assets/img_lv_demo_music_btn_list_play_large.c \
	ext/lvgl/demos/music/assets/img_lv_demo_music_btn_loop.c \
	ext/lvgl/demos/music/assets/img_lv_demo_music_btn_loop_large.c \
	ext/lvgl/demos/music/assets/img_lv_demo_music_btn_next.c \
	ext/lvgl/demos/music/assets/img_lv_demo_music_btn_next_large.c \
	ext/lvgl/demos/music/assets/img_lv_demo_music_btn_pause.c \
	ext/lvgl/demos/music/assets/img_lv_demo_music_btn_pause_large.c \
	ext/lvgl/demos/music/assets/img_lv_demo_music_btn_play.c \
	ext/lvgl/demos/music/assets/img_lv_demo_music_btn_play_large.c \
	ext/lvgl/demos/music/assets/img_lv_demo_music_btn_prev.c \
	ext/lvgl/demos/music/assets/img_lv_demo_music_btn_prev_large.c \
	ext/lvgl/demos/music/assets/img_lv_demo_music_btn_rnd.c \
	ext/lvgl/demos/music/assets/img_lv_demo_music_btn_rnd_large.c \
	ext/lvgl/demos/music/assets/img_lv_demo_music_corner_left.c \
	ext/lvgl/demos/music/assets/img_lv_demo_music_corner_left_large.c \
	ext/lvgl/demos/music/assets/img_lv_demo_music_corner_right.c \
	ext/lvgl/demos/music/assets/img_lv_demo_music_corner_right_large.c \
	ext/lvgl/demos/music/assets/img_lv_demo_music_cover_1.c \
	ext/lvgl/demos/music/assets/img_lv_demo_music_cover_1_large.c \
	ext/lvgl/demos/music/assets/img_lv_demo_music_cover_2.c \
	ext/lvgl/demos/music/assets/img_lv_demo_music_cover_2_large.c \
	ext/lvgl/demos/music/assets/img_lv_demo_music_cover_3.c \
	ext/lvgl/demos/music/assets/img_lv_demo_music_cover_3_large.c \
	ext/lvgl/demos/music/assets/img_lv_demo_music_icon_1.c \
	ext/lvgl/demos/music/assets/img_lv_demo_music_icon_1_large.c \
	ext/lvgl/demos/music/assets/img_lv_demo_music_icon_2.c \
	ext/lvgl/demos/music/assets/img_lv_demo_music_icon_2_large.c \
	ext/lvgl/demos/music/assets/img_lv_demo_music_icon_3.c \
	ext/lvgl/demos/music/assets/img_lv_demo_music_icon_3_large.c \
	ext/lvgl/demos/music/assets/img_lv_demo_music_icon_4.c \
	ext/lvgl/demos/music/assets/img_lv_demo_music_icon_4_large.c \
	ext/lvgl/demos/music/assets/img_lv_demo_music_list_border.c \
	ext/lvgl/demos/music/assets/img_lv_demo_music_list_border_large.c \
	ext/lvgl/demos/music/assets/img_lv_demo_music_logo.c \
	ext/lvgl/demos/music/assets/img_lv_demo_music_slider_knob.c \
	ext/lvgl/demos/music/assets/img_lv_demo_music_slider_knob_large.c \
	ext/lvgl/demos/music/assets/img_lv_demo_music_wave_bottom.c \
	ext/lvgl/demos/music/assets/img_lv_demo_music_wave_bottom_large.c \
	ext/lvgl/demos/music/assets/img_lv_demo_music_wave_top.c \
	ext/lvgl/demos/music/assets/img_lv_demo_music_wave_top_large.c \



OBJS := ${SOURCES:.c=.o}

CFLAGS += -g -O0 -DDEBUG -fmax-errors=3
CFLAGS += -DMUSIC_DEMO

INCLUDEPATH = \
 	-I . \
 	-I lvgl-port \
	-I app \
 	-I ext/lvgl \
	-I ext/lvgl/src/hal \
	-I ext/lvgl/src/misc \
	-I ext/lvgl/demos \
	-I ext/lvgl/demos/music \
	-I ext/lvgl/demos/music/assets \

CFLAGS += ${INCLUDEPATH}
CFLAGS +=  -DUSE_APP_LOG

LDFLAGS += -L . -L /ext

C=gcc

%.o: %.c
	$(C) $(CFLAGS) -fPIC $(INCLUDE) -c $< -o $@

$(APP_NAME): $(LVGL_LIB) $(OBJS) 
	g++ $(CFLAGS) $(OBJS) -l:$(LVGL_LIB) $(LDFLAGS) -l pthread -o $@

clean:
	make -C ext clean
	find . -type f -name '*.o' -delete



