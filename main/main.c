#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_gc9a01.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_commands.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "lvgl.h"
#include "esp_lvgl_port.h"
#include "panel.h"

#include "lv_demos.h"

#define LVGL_TICK_PERIOD_MS 2
static const char *TAG = "main";
lv_disp_t *disp = NULL;

void app_main(void)
{
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));

    gpio_config_t lcd_en_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << LCD_PIN_NUM_EN,
    };
    ESP_ERROR_CHECK(gpio_config(&lcd_en_gpio_config));
    gpio_set_level(LCD_PIN_NUM_EN, 0);

    ESP_LOGI(TAG, "Turn off LCD backlight");
    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << LCD_PIN_NUM_PWM_BL,
    };
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));
    gpio_set_level(LCD_PIN_NUM_PWM_BL, 0);
    ESP_LOGI(TAG, "Initialize Intel 8080 bus");

    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_io_handle_t io_handle = NULL;
    display_init(&panel_handle, &io_handle);

    esp_lcd_touch_handle_t touch_handle = NULL;
    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    touch_init(&touch_handle, &tp_io_handle);

    ESP_LOGI(TAG, "Initialize LVGL library");
    
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = LCD_H_RES*LCD_V_RES,
        .double_buffer = true,
        .hres = LCD_H_RES,
        .vres = LCD_V_RES,
        .monochrome = false,
        /* Rotation values must be same as used in esp_lcd for initial settings of the screen */
        .rotation = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        },
        .flags = {
            .buff_dma = true,
        }
    };
    disp = lvgl_port_add_disp(&disp_cfg);

    const lvgl_port_touch_cfg_t touch_cfg = {
        .disp = disp,
        .handle = touch_handle,
    };
    lv_indev_t* lv_touch_handle = lvgl_port_add_touch(&touch_cfg);

    ESP_LOGI(TAG, "Display LVGL animation");
    lvgl_port_lock(0);
    lv_demo_music();
    lvgl_port_unlock();

    ESP_LOGI(TAG, "Turn on LCD backlight");
    gpio_set_level(LCD_PIN_NUM_PWM_BL, 1);
}