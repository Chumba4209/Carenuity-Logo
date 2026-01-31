#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

extern "C" {
  #include "lvgl.h"
}

// IMAGE
extern const lv_image_dsc_t carenuity_logo;

// TFT

TFT_eSPI tft = TFT_eSPI();

// TOUCH

#define XPT2046_IRQ   36
#define XPT2046_MOSI  32
#define XPT2046_MISO  39
#define XPT2046_CLK   25
#define XPT2046_CS    33

SPIClass touchscreenSPI = SPIClass(VSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);



// DISPLAY
#define SCREEN_WIDTH   240
#define SCREEN_HEIGHT  320

static lv_display_t *disp;

/* Small buffer for partial rendering */
static lv_color_t draw_buf[SCREEN_WIDTH * 40];



// GLOBALS

int x, y, z;

lv_obj_t *screen_logo;
lv_obj_t *screen_touch;



// LVGL TICK CALLBACK (LVGL 9 REQUIRES THIS)

uint32_t my_tick_cb(void)
{
  return millis();
}



// TOUCH READ CALLBACK

void touchscreen_read(lv_indev_t * indev, lv_indev_data_t * data)
{
  if (touchscreen.tirqTouched() && touchscreen.touched()) {
    TS_Point p = touchscreen.getPoint();

    x = map(p.x, 200, 3700, 1, SCREEN_WIDTH);
    y = map(p.y, 240, 3800, 1, SCREEN_HEIGHT);

    data->state = LV_INDEV_STATE_PRESSED;
    data->point.x = x;
    data->point.y = y;
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}



// DISPLAY FLUSH CALLBACK

void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
  uint32_t w = area->x2 - area->x1 + 1;
  uint32_t h = area->y2 - area->y1 + 1;

  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors((uint16_t *)px_map, w * h, true);
  tft.endWrite();

  lv_display_flush_ready(disp);
}



// UI SCREENS
void create_logo_screen()
{
  screen_logo = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(screen_logo, lv_color_white(), 0);
  lv_obj_set_style_bg_opa(screen_logo, LV_OPA_COVER, 0);

  // Logo image ONLY (logo + text already combined)
  lv_obj_t *img = lv_img_create(screen_logo);
  lv_img_set_src(img, &carenuity_logo);
  lv_img_set_zoom(img, 180);
  lv_obj_center(img);

  // Touch anywhere → switch screen
  lv_obj_add_event_cb(
    screen_logo,
    [](lv_event_t * e) {
      lv_scr_load(screen_touch);
    },
    LV_EVENT_PRESSED,
    NULL
  );
}

void create_touch_screen()
{
  screen_touch = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(screen_touch, lv_color_white(), 0);
  lv_obj_set_style_bg_opa(screen_touch, LV_OPA_COVER, 0);

  lv_obj_t *title = lv_label_create(screen_touch);
  lv_label_set_text(title, "Carenuity");
  lv_obj_set_style_text_font(title, &lv_font_montserrat_48, 0);
  lv_obj_set_style_text_color(title, lv_color_black(), 0);
  lv_obj_align(title, LV_ALIGN_CENTER, 0, -20);

  lv_obj_t *footer = lv_label_create(screen_touch);
  lv_label_set_text(footer, "by Cynthia");
  lv_obj_set_style_text_font(footer, &lv_font_montserrat_18, 0);
  lv_obj_set_style_text_color(footer, lv_color_black(), 0);
  lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, -10);
}

void lv_create_main_gui()
{
  create_touch_screen();
  create_logo_screen();
  lv_scr_load(screen_logo);
}



// SETUP


void setup()
{
  Serial.begin(115200);

  //  LVGL INIT 
  lv_init();
  lv_tick_set_cb(my_tick_cb);

  //  TFT INIT 
  tft.init();
  tft.setRotation(1);

  // Debug color test
  tft.fillScreen(TFT_RED);
  delay(300);
  tft.fillScreen(TFT_GREEN);
  delay(300);
  tft.fillScreen(TFT_BLUE);
  delay(300);
  tft.fillScreen(TFT_BLACK);

  //  TOUCH INIT 
  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(touchscreenSPI);
  touchscreen.setRotation(2);

  //  DISPLAY DRIVER 
  disp = lv_display_create(SCREEN_WIDTH, SCREEN_HEIGHT);
  lv_display_set_flush_cb(disp, my_disp_flush);

  lv_display_set_buffers(
    disp,
    draw_buf,
    NULL,
    sizeof(draw_buf),
    LV_DISPLAY_RENDER_MODE_PARTIAL
  );

  lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_270);

  //  INPUT DEVICE 
  lv_indev_t *indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, touchscreen_read);

  //  GUI 
  lv_create_main_gui();
}



// LOOP (LVGL 9)

void loop()
{
  lv_timer_handler();
  delay(5);
}