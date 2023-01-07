/***************************************************************
 * 
 *  MIT License: https://mit-license.org/
 *
 *  Copyright © 2022 Varanda Labs Inc.
 * 
 ***************************************************************
 */

#include "lvgl.h"
#include "lv_hal_disp.h"
#include "log.h"
#include <pthread.h>
#include <unistd.h>

#include <stdio.h>
#include <syslog.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <linux/input.h>
#include <sys/eventfd.h>
#include <sys/poll.h>
#include <errno.h>
//#include <cairo.h>

extern void lvgl_app_main (void);
extern void lv_demo_music(void);
extern void ui_init(void);

#define RED         0b1111100000000000
#define GREEN       0b0000011111100000
#define BLUE        0b0000000000011111

#define FB_DEV_NAME "/dev/fb1"
#define EVENT_DEV_NAME "/dev/input/event2"

#define TYPE__EV_ABS              3
#define CODE__ABS_X               1
#define CODE__ABS_Y               0
#define CODE__ABS_PRESSURE        24

#define ABS_X__MIN_VALUE          0       // Left
#define ABS_X__MAX_VALUE          4095
#define ABS_Y__MIN_VALUE          0
#define ABS_Y__MAX_VALUE          4095    // top

static int fbfd = -1;
static char *fbp = 0;
static bool exit_flag = false;
static int event_fd = -1;

static volatile bool touch_in_progress = false;


pthread_t event_moni_thread_id = -1;

#define LVGL_TICK_TIME 1 //  1 milli

static void updateDisplay (const lv_area_t * area, lv_color_t * color_p, bool last);

#define DISP_BUF_SIZE (LV_HOR_RES_MAX * 10)

static int touchpad_x = 0, touchpad_y = 0;
static lv_indev_state_t touchpad_state = LV_INDEV_STATE_REL;
static lv_indev_state_t touchpad_old_state = LV_INDEV_STATE_REL;

static void disp_flush(lv_disp_drv_t * disp, const lv_area_t * area, lv_color_t * color_p);
static bool touchpad_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data);

static struct fb_var_screeninfo vinfo;
static struct fb_fix_screeninfo finfo;

static lv_indev_drv_t indev_drv;
lv_indev_t * global_indev;

static int fbwriter_open(char * dev_name) 
{
    fbfd = open(dev_name, O_RDWR);
    if (fbfd == -1) {
        LOG_E( "Unable to open secondary display\n");
        return -1;
    }
    if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo)) {
        LOG_E( "Unable to get secondary display information\n");
        return -1;
    }
    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo)) {
        LOG_E( "Unable to get secondary display information\n");
        return -1;
    }

    LOG("framebuffer display is %d x %d %dbps\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);

    fbp = (char*) mmap(0, finfo.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
    if (! fbp) {
        LOG_E( "Unable to create memory mapping");
        close(fbfd);
        return -1;
    }
    return 0;
}

static int fbwriter_update(char * rgb565_ptr)
{
    memcpy(fbp, rgb565_ptr,  vinfo.xres * vinfo.yres * (vinfo.bits_per_pixel / 8));
}

static void fbwriter_close()
{
    LOG("loop done");
    munmap(fbp, finfo.smem_len);
    close(fbfd);
}

/*
 refs:
     https://learn.watterott.com/hats/rpi-display/fbtft-install/
     events: https://www.kernel.org/doc/html/v4.15/input/input.html
             see with either "cat /dev/input/event2 |xxd" or evtest
             code: https://github.com/Robin329/evtest/blob/master/evtest.c

Input device name: "ADS7846 Touchscreen"
Supported events:
  Event type 0 (EV_SYN)
  Event type 1 (EV_KEY)
    Event code 330 (BTN_TOUCH)
  Event type 3 (EV_ABS)
    Event code 0 (ABS_X)
      Value   3222
      Min        0
      Max     4095
    Event code 1 (ABS_Y)
      Value   1713
      Min        0
      Max     4095
    Event code 24 (ABS_PRESSURE)
      Value      0
      Min        0
      Max      255

Top Left:
Event: time 1671994700.406903, -------------- SYN_REPORT ------------
Event: time 1671994700.418902, type 3 (EV_ABS), code 0 (ABS_X), value 337
Event: time 1671994700.418902, type 3 (EV_ABS), code 1 (ABS_Y), value 3878
Event: time 1671994700.418902, type 3 (EV_ABS), code 24 (ABS_PRESSURE), value 128
Event: time 1671994700.418902, -------------- SYN_REPORT ------------
Event: time 1671994700.430727, type 1 (EV_KEY), code 330 (BTN_TOUCH), value 0
Event: time 1671994700.430727, type 3 (EV_ABS), code 24 (ABS_PRESSURE), value 0

Top Right:
Event: time 1671994761.254899, -------------- SYN_REPORT ------------
Event: time 1671994761.266915, type 3 (EV_ABS), code 0 (ABS_X), value 326
Event: time 1671994761.266915, type 3 (EV_ABS), code 1 (ABS_Y), value 282
Event: time 1671994761.266915, type 3 (EV_ABS), code 24 (ABS_PRESSURE), value 118
Event: time 1671994761.266915, -------------- SYN_REPORT ------------
Event: time 1671994761.278716, type 1 (EV_KEY), code 330 (BTN_TOUCH), value 0
Event: time 1671994761.278716, type 3 (EV_ABS), code 24 (ABS_PRESSURE), value 0

Bottom Left:
Event: time 1671994798.310904, -------------- SYN_REPORT ------------
Event: time 1671994798.322893, type 3 (EV_ABS), code 0 (ABS_X), value 3787
Event: time 1671994798.322893, type 3 (EV_ABS), code 1 (ABS_Y), value 3923
Event: time 1671994798.322893, type 3 (EV_ABS), code 24 (ABS_PRESSURE), value 147
Event: time 1671994798.322893, -------------- SYN_REPORT ------------
Event: time 1671994798.334717, type 1 (EV_KEY), code 330 (BTN_TOUCH), value 0
Event: time 1671994798.334717, type 3 (EV_ABS), code 24 (ABS_PRESSURE), value 0

Bottom Right:
Event: time 1671994848.038901, -------------- SYN_REPORT ------------
Event: time 1671994848.050911, type 3 (EV_ABS), code 0 (ABS_X), value 3769
Event: time 1671994848.050911, type 3 (EV_ABS), code 1 (ABS_Y), value 280
Event: time 1671994848.050911, type 3 (EV_ABS), code 24 (ABS_PRESSURE), value 144
Event: time 1671994848.050911, -------------- SYN_REPORT ------------
Event: time 1671994848.062729, type 1 (EV_KEY), code 330 (BTN_TOUCH), value 0
Event: time 1671994848.062729, type 3 (EV_ABS), code 24 (ABS_PRESSURE), value 0

*/

static bool touchpad_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
  struct input_event ev;
  int rd;
  static int x = 0;
  static int y = 0;

  while(read(event_fd, &ev, sizeof(ev)) == sizeof(ev)) {

    if (ev.type == TYPE__EV_ABS) {
      switch(ev.code) {
        case CODE__ABS_X:
          x = (ev.value * LV_HOR_RES_MAX) / ABS_X__MAX_VALUE;
          x = LV_HOR_RES_MAX - x;
          break;

        case CODE__ABS_Y:
          y = (ev.value * LV_VER_RES_MAX) / ABS_Y__MAX_VALUE;
          break;

        case CODE__ABS_PRESSURE:
          if (ev.value > 0) {
            touch_in_progress = true;
            break;
          }
          data->state = LV_INDEV_STATE_REL;
          touch_in_progress = false;
          break;

        default:
          LOG("unexpected event code: %d\n", ev.code);
          break;
      }
    }
  }
  data->point.x = x;
  data->point.y = y;
  if (touch_in_progress) {
    data->state = LV_INDEV_STATE_PR;
    //LOG("(%d , %d)\n", x, y);
  }
  else {
    data->state = LV_INDEV_STATE_REL;
  }
  
  return false;
}

static void init_pointer(void)
{
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = (void (*)(struct _lv_indev_drv_t *, lv_indev_data_t * )) touchpad_read;
    global_indev = lv_indev_drv_register(&indev_drv);
}

static void init_disp()
{
  static lv_disp_draw_buf_t draw_buf_dsc_1;
  static lv_color_t buf_1[DISP_BUF_SIZE];                          /*A buffer for 10 rows*/
  lv_disp_draw_buf_init(&draw_buf_dsc_1, buf_1, NULL, DISP_BUF_SIZE);   /*Initialize the display buffer*/

  static lv_disp_drv_t disp_drv;                         /*Descriptor of a display driver*/
  lv_disp_drv_init(&disp_drv);                    /*Basic initialization*/

  /*Set up the functions to access to your display*/

  /*Set the resolution of the display*/
  disp_drv.hor_res = LV_HOR_RES_MAX;
  disp_drv.ver_res = LV_VER_RES_MAX;

  /*Used to copy the buffer's content to the display*/
  disp_drv.flush_cb = disp_flush;

  /*Set a display buffer*/
  disp_drv.draw_buf = &draw_buf_dsc_1;

#if LV_USE_GPU
  /*Fill a memory array with a color*/
  disp_drv.gpu_fill_cb = gpu_fill;
#endif

  /*Finally register the driver*/
  lv_disp_drv_register(&disp_drv);
}

void lv_integr_update_pointer(int x, int y, int state)
{
  touchpad_x = x;
  touchpad_y = y;
  touchpad_state = (lv_indev_state_t) state;
}

// Must be called from main thread
void * lv_integr_timer(void * arg) {
  while( ! exit_flag) {
    static int cnt = 0;
    lv_tick_inc(LVGL_TICK_TIME);

    if (cnt++ > 4) {
      cnt = 0;
      lv_task_handler();
    }
    // if ( ! touch_in_progress)
    usleep(1000 * LVGL_TICK_TIME);
  }

  return NULL;
}

static void disp_flush(lv_disp_drv_t * disp, const lv_area_t * area, lv_color_t * color_p)
{
  bool last = lv_disp_flush_is_last( disp);
  updateDisplay(area, color_p, last);
  lv_disp_flush_ready(disp);         /* Indicate you are ready with the flushing*/
}

static void updateDisplay (const lv_area_t * area, lv_color_t * color_p, bool last)
{
    int32_t x, y;

    uint16_t * p = (uint16_t *) fbp;
    uint16_t pixel_output = 0;
    for(y = area->y1; y <= area->y2; y++) {
        for(x = area->x1; x <= area->x2; x++) {
            *(p + (x + y * LV_HOR_RES_MAX)) = (*color_p).full; 
            color_p++;
        }
    }

}

static void sigterm_handler(int s)
{
  exit_flag = true;
}

int main(int argc, char **argv)
{
	if (signal(SIGTERM, sigterm_handler) == SIG_ERR) // add signal to handle systemctl stop
	{
		LOG_W("fail to set signal SIGTERM\n");
	}
	if (signal(SIGINT, sigterm_handler) == SIG_ERR) // add signal to handle Control-C
	{
		LOG_W("fail to set signal SIGINT\n");
	}

  LOG("runNative from LOG");
  if (fbwriter_open(FB_DEV_NAME)) {
    return 1;
  }
  lv_init();
  init_disp();
  init_pointer();
  int res;

  event_fd = open(EVENT_DEV_NAME, O_RDONLY | O_NONBLOCK);

#if defined(GUITAR_PEDALS)
  ui_init();
#elif defined(MUSIC_DEMO)
  lv_demo_music();
#else
  lvgl_app_main();
#endif

  lv_integr_timer(NULL);

  close(event_fd);
  fbwriter_close();

  LOG("lvgl_app terminated\n");
  return 0;
}


