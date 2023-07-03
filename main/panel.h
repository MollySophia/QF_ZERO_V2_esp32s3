#ifndef _PANEL_H
#define _PANEL_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LCD_H_RES         (240)
#define LCD_V_RES         (240)

esp_err_t display_init(esp_lcd_panel_handle_t *panel_handle, esp_lcd_panel_io_handle_t *io_handle);
esp_err_t touch_init(esp_lcd_touch_handle_t *touch_handle, esp_lcd_panel_io_handle_t *io_handle);

#ifdef __cplusplus
}
#endif

#endif
