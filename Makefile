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

SOURCES = \
	app/app.c \
	lvgl-port/integration.c \

OBJS := ${SOURCES:.c=.o}

CFLAGS += -g -O0 -DDEBUG -fmax-errors=3

INCLUDEPATH = \
 	-I . \
 	-I lvgl-port \
 	-I ext/lvgl \
	-I ext/lvgl/src/hal \

CFLAGS += ${INCLUDEPATH}

LDFLAGS += -L . -L /ext

C=gcc

%.o: %.c
	$(C) $(CFLAGS) -fPIC $(INCLUDE) -c $< -o $@

$(APP_NAME): $(LVGL_LIB) $(OBJS) 
	g++ $(CFLAGS) $(OBJS) -l:$(LVGL_LIB) $(LDFLAGS) -o $@



