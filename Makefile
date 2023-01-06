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
	app/app.c \


OBJS := ${SOURCES:.c=.o}

CFLAGS += -g -O0 -DDEBUG -fmax-errors=3

INCLUDEPATH = \
 	-I . \
 	-I lvgl-port \
	-I app \
 	-I ext/lvgl \
	-I ext/lvgl/src/hal \
	-I ext/lvgl/src/misc \

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



