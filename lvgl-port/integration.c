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
//#include <cairo.h>

extern void lvgl_app_main (void);

#define RED         0b1111100000000000
#define GREEN       0b0000011111100000
#define BLUE        0b0000000000011111

#define FB_DEV_NAME "/dev/fb1"

static int fbfd = -1;
static char *fbp = 0;
static bool exit_flag = false;

pthread_t timer_thread_id = -1;

#define LVGL_TICK_TIME 5 //  5 milli

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


static bool touchpad_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
#if 1
  //LOG("touchpad_read\n");
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
  return NULL;
}

static void disp_flush(lv_disp_drv_t * disp, const lv_area_t * area, lv_color_t * color_p)
{
  bool last = lv_disp_flush_is_last( disp);
  updateDisplay(area, color_p, last);
  lv_disp_flush_ready(disp);         /* Indicate you are ready with the flushing*/
}


//----------- C++ functions ---------

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

  lvgl_app_main();
  lv_integr_timer(NULL);

  fbwriter_close();
  LOG("lvgl_app terminated\n");
  return 0;
}


