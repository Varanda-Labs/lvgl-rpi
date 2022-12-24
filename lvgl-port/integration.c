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

pthread_t timer_thread_id = -1;

#define LVGL_TICK_TIME 50 //  50 milli

static void updateDisplay (const lv_area_t * area, lv_color_t * color_p, bool last);

#define DISP_BUF_SIZE (LV_HOR_RES_MAX * 10)

static lv_indev_drv_t indev_drv;
static int touchpad_x = 0, touchpad_y = 0;
static lv_indev_state_t touchpad_state = LV_INDEV_STATE_REL;
static lv_indev_state_t touchpad_old_state = LV_INDEV_STATE_REL;

static void disp_flush(lv_disp_drv_t * disp, const lv_area_t * area, lv_color_t * color_p);
static bool touchpad_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data);

static bool touchpad_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
  bool ret = false;
  data->point.x = touchpad_x;
  data->point.y = touchpad_y;
  data->state = touchpad_state; //LV_INDEV_STATE_REL; //LV_INDEV_STATE_PR or LV_INDEV_STATE_REL;
  if ( touchpad_state != touchpad_old_state) {
      touchpad_old_state = touchpad_state;
      LOG("mouse down: x=%d y=%d", touchpad_x, touchpad_y);
  }
  return ret; /*No buffering now so no more data read*/
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

void * lv_integr_timer(void * arg) {
  while(1) {
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
    lv_color_t pixel;
#if 0
    QRgb pixel_output;

    for(y = area->y1; y <= area->y2; y++) {
        for(x = area->x1; x <= area->x2; x++) {
            pixel = *color_p;
            pixel_output = pixel.ch.red << (16 + 3);
            pixel_output |= pixel.ch.green << (8 + 2);
            pixel_output |= pixel.ch.blue << 3;

            gMainObj->display_image.setPixelColor(x,y, pixel_output);
            color_p++;
        }
    }
    if (last) {
        gMainObj->ui->lb_display->setPixmap(QPixmap::fromImage(gMainObj->display_image));
    }
#endif

}

int main(int argc, char **argv)
{
  LOG("runNative from LOG");

  lv_init();
  init_disp();
  init_pointer();
  int res;
  if((res = pthread_create((pthread_t *) &timer_thread_id, NULL, lv_integr_timer, NULL)) == 0) {
    pthread_join(timer_thread_id, NULL);
  }
  else {
    LOG_E("Could not create timer thread\nlvgl_app terminated.\n");
    return 1;
  }

  LOG("lvgl_app terminated\n");
  return 0;
}


