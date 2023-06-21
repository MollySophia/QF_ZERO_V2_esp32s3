#ifndef _PANEL_H
#define _PANEL_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LCD_H_RES         (240)
#define LCD_V_RES         (240)

#define LCD_PIN_NUM_PWM_BL (45)
#define LCD_PIN_NUM_EN    (41)
#define LCD_PIN_NUM_CLK   (7)
#define LCD_PIN_NUM_RST   (8)
#define LCD_PIN_NUM_CS    (14)
#define LCD_PIN_NUM_DC    (13)
#define LCD_PIN_NUM_DATA0 (6)
#define LCD_PIN_NUM_DATA1 (12)
#define LCD_PIN_NUM_DATA2 (5)
#define LCD_PIN_NUM_DATA3 (11)
#define LCD_PIN_NUM_DATA4 (4)
#define LCD_PIN_NUM_DATA5 (10)
#define LCD_PIN_NUM_DATA6 (3)
#define LCD_PIN_NUM_DATA7 (9)

#define TP_PIN_NUM_SDA    (15)
#define TP_PIN_NUM_SCL    (16)
#define TP_PIN_NUM_INT    (17)

esp_err_t display_init(esp_lcd_panel_handle_t *panel_handle, esp_lcd_panel_io_handle_t *io_handle);
esp_err_t touch_init(esp_lcd_touch_handle_t *touch_handle, esp_lcd_panel_io_handle_t *io_handle);

#ifdef __cplusplus
}
#endif

#endif
