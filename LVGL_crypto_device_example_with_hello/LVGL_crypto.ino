#include "lcd_bsp.h"
#include "FT3168.h"
void setup()
{
  Serial.begin(115200);
  Touch_Init();
  lcd_lvgl_Init();

  lv_obj_t *label = lv_label_create(lv_scr_act());
  lv_label_set_text(label, "Hello World v2.0");
  lv_obj_center(label);
}

void loop()
{
  //delay(1000);
  //set_amoled_backlight(0xff);
  //delay(1000);
  //set_amoled_backlight(200);
  //delay(1000);
  //set_amoled_backlight(150);
  //delay(1000);
  //set_amoled_backlight(100);
  //delay(1000);
  //set_amoled_backlight(50);
  //delay(1000);
  //set_amoled_backlight(0);
}
