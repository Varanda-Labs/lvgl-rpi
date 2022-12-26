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

#define RED         0b1111100000000000
#define GREEN       0b0000011111100000
#define BLUE        0b0000000000011111

#define FB_DEV_NAME "/dev/fb1"
#define EVENT_DEV_NAME "/dev/input/event2"

static int fbfd = -1;
static char *fbp = 0;
static bool exit_flag = false;

#ifdef USE_EVENT_THREAD
  static int ternimate_fd = -1; // eventfd(0, EFD_NONBLOCK);
#else
  static int event_fd = -1;
#endif

pthread_t event_moni_thread_id = -1;

#define LVGL_TICK_TIME 1 //  1 milli

static void updateDisplay (const lv_area_t * area, lv_color_t * color_p, bool last);

#define DISP_BUF_SIZE (LV_HOR_RES_MAX * 10)

static lv_indev_drv_t indev_drv;
static int touchpad_x = 0, touchpad_y = 0;
static lv_indev_state_t touchpad_state = LV_INDEV_STATE_REL;
static lv_indev_state_t touchpad_old_state = LV_INDEV_STATE_REL;

static void disp_flush(lv_disp_drv_t * disp, const lv_area_t * area, lv_color_t * color_p);
static bool touchpad_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data);

static struct fb_var_screeninfo vinfo;
static struct fb_fix_screeninfo finfo;

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

#define TYPE__EV_ABS              3
#define CODE__ABS_X               1
#define CODE__ABS_Y               0
#define CODE__ABS_PRESSURE        24

#define ABS_X__MIN_VALUE          0       // Left
#define ABS_X__MAX_VALUE          4095
#define ABS_Y__MIN_VALUE          0
#define ABS_Y__MAX_VALUE          4095    // top


/**
struct input_event {
	struct timeval time;
	unsigned short type;
	unsigned short code;
	unsigned int value;
};

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

#ifdef USE_EVENT_THREAD

static inline const char* typename(unsigned int type)
{
  return "TYPE ???";
	//return (type <= EV_MAX && events[type]) ? events[type] : "?";
}

static inline const char* codename(unsigned int type, unsigned int code)
{
    return "CODE ???";
	//return (type <= EV_MAX && code <= maxval[type] && names[type] && names[type][code]) ? names[type][code] : "?";
}


static void * event_monitor_thread(void * par)
{
  struct input_event ev;
  int	rd, ret;
  struct pollfd fds[2];

  int fd = open(EVENT_DEV_NAME, O_RDONLY);
  if (!fd) {
    LOG_E("Could not open event file\n");
    return NULL;
  }

  ternimate_fd = eventfd(0, EFD_NONBLOCK);
  if (ternimate_fd == -1 ) {
    LOG_E("Could not initialize ternimate_fd, err: %s\n", strerror(errno));
    close(fd);
    return NULL;
  }

  memset(fds, 0, sizeof(fds));
  fds[0].fd = ternimate_fd;
  fds[1].fd = fd;

  fds[0].events = POLLIN;
  fds[1].events = POLLIN;


  while(1) {
    ret = poll(fds, 2, -1);

    if (ret <= 0) {
      LOG_E("poll fail\n");
      close(fd);
      close(ternimate_fd);
      return NULL;
    }

    if (fds[0].revents & POLLIN) {
      break;
    }

    if (fds[1].revents & POLLIN == 0) {
      LOG("Unexpected state...\n");
      continue;
    }

    if (exit_flag) {
      LOG("leaving\n");
      break;
    }

	  rd = read(fd, &ev, sizeof(ev));

		if (rd < (int) sizeof(struct input_event)) {
			LOG_E("expected %d bytes, got %d\n", (int) sizeof(struct input_event), rd);
			LOG_E("\nevtest: error reading, terminating event monitor thread");
			return NULL;
		}

    LOG("Event: time %ld.%06ld, ", ev.input_event_sec, ev.input_event_usec);
    int type = ev.type;
    int code = ev.code;

    if (type == EV_SYN) {
      if (code == SYN_MT_REPORT)
        LOG("++++++++++++++ %s ++++++++++++\n", codename(type, code));
      else if (code == SYN_DROPPED)
        LOG(">>>>>>>>>>>>>> %s <<<<<<<<<<<<\n", codename(type, code));
      else
        LOG("-------------- %s ------------\n", codename(type, code));
    } else {
      LOG("type %d (%s), code %d (%s), ",
        type, typename(type),
        code, codename(type, code));
      if (type == EV_MSC && (code == MSC_RAW || code == MSC_SCAN))
        LOG("value %02x\n", ev.value);
      else
        LOG("value %d\n", ev.value);
    }

  }

  LOG("terminate event monitor thread.\n");
  close(fd);
  close(ternimate_fd);

}
#endif

static bool touchpad_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
#if 1
  //LOG("touchpad_read\n");
  struct input_event ev;
  int rd;
  while(read(event_fd, &ev, sizeof(ev)) == sizeof(ev)) {
    //rd = read(event_fd, &ev, sizeof(ev));
    // if (rd != sizeof(ev)) {
    //   return false;
    // }

    static int x = 0;
    static int y = 0;
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
            LOG("(%d , %d)\n", x, y);
            data->point.x = x;
            data->point.y = y;
            data->state = LV_INDEV_STATE_PR;
            return true;
          }
          data->state = LV_INDEV_STATE_REL;
          break;

        default:
          LOG("unexpected event code: %d\n", ev.code);
          break;
      }
    }
  }
  return false;

#else
  bool ret = false;
  data->point.x = touchpad_x;
  data->point.y = touchpad_y;
  data->state = touchpad_state; //LV_INDEV_STATE_REL; //LV_INDEV_STATE_PR or LV_INDEV_STATE_REL;
  if ( touchpad_state != touchpad_old_state) {
      touchpad_old_state = touchpad_state;
      LOG("mouse down: x=%d y=%d", touchpad_x, touchpad_y);
  }
  return ret; /*No buffering now so no more data read*/
#endif
}

static void init_pointer(void)
{
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = (void (*)(struct _lv_indev_drv_t *, lv_indev_data_t * )) touchpad_read;
  lv_indev_drv_register(&indev_drv);
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
    usleep(1000 * LVGL_TICK_TIME);
  }

#ifdef USE_EVENT_THREAD
  if (ternimate_fd != -1) {
    uint64_t v = 0x30;
    write(ternimate_fd, &v, sizeof(v));
  }
#endif

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

#ifdef USE_EVENT_THREAD
  if((res = pthread_create((pthread_t *) &event_moni_thread_id, NULL, event_monitor_thread, NULL))) {
    LOG_E("Could not create timer thread\nlvgl_app terminated.\n");
    fbwriter_close();
    return 1;
  }
#else
  event_fd = open(EVENT_DEV_NAME, O_RDONLY | O_NONBLOCK);
#endif

  lvgl_app_main();
  lv_integr_timer(NULL);

  fbwriter_close();
  LOG("Wait event thread to terminate...\n");

#ifdef USE_EVENT_THREAD
  pthread_join(event_moni_thread_id, NULL);
#endif

  LOG("lvgl_app terminated\n");
  return 0;
}


