/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <sys/cdefs.h>
#include "sdkconfig.h"
#if CONFIG_LCD_ENABLE_DEBUG_LOG
// The local log level must be defined before including esp_log.h
// Set the maximum log level for this source file
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#endif
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_lcd_panel_interface.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_gc9a01.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_commands.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_check.h"

static const char *TAG = "lcd_panel.gc9a01";

static esp_err_t panel_gc9a01_del(esp_lcd_panel_t *panel);
static esp_err_t panel_gc9a01_reset(esp_lcd_panel_t *panel);
static esp_err_t panel_gc9a01_init(esp_lcd_panel_t *panel);
static esp_err_t panel_gc9a01_draw_bitmap(esp_lcd_panel_t *panel, int x_start, int y_start, int x_end, int y_end, const void *color_data);
static esp_err_t panel_gc9a01_invert_color(esp_lcd_panel_t *panel, bool invert_color_data);
static esp_err_t panel_gc9a01_mirror(esp_lcd_panel_t *panel, bool mirror_x, bool mirror_y);
static esp_err_t panel_gc9a01_swap_xy(esp_lcd_panel_t *panel, bool swap_axes);
static esp_err_t panel_gc9a01_set_gap(esp_lcd_panel_t *panel, int x_gap, int y_gap);
static esp_err_t panel_gc9a01_disp_on_off(esp_lcd_panel_t *panel, bool off);

typedef struct {
    esp_lcd_panel_t base;
    esp_lcd_panel_io_handle_t io;
    int reset_gpio_num;
    bool reset_level;
    int x_gap;
    int y_gap;
    uint8_t fb_bits_per_pixel;
    uint8_t madctl_val; // save current value of LCD_CMD_MADCTL register
    uint8_t colmod_cal; // save surrent value of LCD_CMD_COLMOD register
} gc9a01_panel_t;

esp_err_t esp_lcd_new_panel_gc9a01(const esp_lcd_panel_io_handle_t io, const esp_lcd_panel_dev_config_t *panel_dev_config, esp_lcd_panel_handle_t *ret_panel)
{
#if CONFIG_LCD_ENABLE_DEBUG_LOG
    esp_log_level_set(TAG, ESP_LOG_DEBUG);
#endif
    esp_err_t ret = ESP_OK;
    gc9a01_panel_t *gc9a01 = NULL;
    ESP_GOTO_ON_FALSE(io && panel_dev_config && ret_panel, ESP_ERR_INVALID_ARG, err, TAG, "invalid argument");
    gc9a01 = calloc(1, sizeof(gc9a01_panel_t));
    ESP_GOTO_ON_FALSE(gc9a01, ESP_ERR_NO_MEM, err, TAG, "no mem for gc9a01 panel");

    if (panel_dev_config->reset_gpio_num >= 0) {
        gpio_config_t io_conf = {
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = 1ULL << panel_dev_config->reset_gpio_num,
        };
        ESP_GOTO_ON_ERROR(gpio_config(&io_conf), err, TAG, "configure GPIO for RST line failed");
    }

    switch (panel_dev_config->rgb_endian) {
    case LCD_RGB_ENDIAN_RGB:
        gc9a01->madctl_val = 0;
        break;
    case LCD_RGB_ENDIAN_BGR:
        gc9a01->madctl_val |= LCD_CMD_BGR_BIT;
        break;
    default:
        ESP_GOTO_ON_FALSE(false, ESP_ERR_NOT_SUPPORTED, err, TAG, "unsupported color space");
        break;
    }

    uint8_t fb_bits_per_pixel = 0;
    switch (panel_dev_config->bits_per_pixel) {
    case 16: // RGB565
        gc9a01->colmod_cal = 0x55;
        fb_bits_per_pixel = 16;
        break;
    case 18: // RGB666
        gc9a01->colmod_cal = 0x66;
        // each color component (R/G/B) should occupy the 6 high bits of a byte, which means 3 full bytes are required for a pixel
        fb_bits_per_pixel = 24;
        break;
    default:
        ESP_GOTO_ON_FALSE(false, ESP_ERR_NOT_SUPPORTED, err, TAG, "unsupported pixel width");
        break;
    }

    gc9a01->io = io;
    gc9a01->fb_bits_per_pixel = fb_bits_per_pixel;
    gc9a01->reset_gpio_num = panel_dev_config->reset_gpio_num;
    gc9a01->reset_level = panel_dev_config->flags.reset_active_high;
    gc9a01->base.del = panel_gc9a01_del;
    gc9a01->base.reset = panel_gc9a01_reset;
    gc9a01->base.init = panel_gc9a01_init;
    gc9a01->base.draw_bitmap = panel_gc9a01_draw_bitmap;
    gc9a01->base.invert_color = panel_gc9a01_invert_color;
    gc9a01->base.set_gap = panel_gc9a01_set_gap;
    gc9a01->base.mirror = panel_gc9a01_mirror;
    gc9a01->base.swap_xy = panel_gc9a01_swap_xy;
    gc9a01->base.disp_on_off = panel_gc9a01_disp_on_off;
    *ret_panel = &(gc9a01->base);
    ESP_LOGD(TAG, "new gc9a01 panel @%p", gc9a01);

    return ESP_OK;

err:
    if (gc9a01) {
        if (panel_dev_config->reset_gpio_num >= 0) {
            gpio_reset_pin(panel_dev_config->reset_gpio_num);
        }
        free(gc9a01);
    }
    return ret;
}

static esp_err_t panel_gc9a01_del(esp_lcd_panel_t *panel)
{
    gc9a01_panel_t *gc9a01 = __containerof(panel, gc9a01_panel_t, base);

    if (gc9a01->reset_gpio_num >= 0) {
        gpio_reset_pin(gc9a01->reset_gpio_num);
    }
    ESP_LOGD(TAG, "del gc9a01 panel @%p", gc9a01);
    free(gc9a01);
    return ESP_OK;
}

static esp_err_t panel_gc9a01_reset(esp_lcd_panel_t *panel)
{
    gc9a01_panel_t *gc9a01 = __containerof(panel, gc9a01_panel_t, base);
    esp_lcd_panel_io_handle_t io = gc9a01->io;

    // perform hardware reset
    if (gc9a01->reset_gpio_num >= 0) {
        gpio_set_level(gc9a01->reset_gpio_num, gc9a01->reset_level);
        vTaskDelay(pdMS_TO_TICKS(10));
        gpio_set_level(gc9a01->reset_gpio_num, !gc9a01->reset_level);
        vTaskDelay(pdMS_TO_TICKS(10));
    } else { // perform software reset
    
    }

    return ESP_OK;
}

static esp_err_t panel_gc9a01_init(esp_lcd_panel_t *panel)
{
    gc9a01_panel_t *gc9a01 = __containerof(panel, gc9a01_panel_t, base);
    esp_lcd_panel_io_handle_t io = gc9a01->io;

    esp_lcd_panel_io_tx_param(io, 0xFE, NULL, 0);
    esp_lcd_panel_io_tx_param(io, 0xEF, NULL, 0);

    esp_lcd_panel_io_tx_param(io, 0xEB, (uint8_t[]){0x14}, 1);
    esp_lcd_panel_io_tx_param(io, 0x84, (uint8_t[]){0x40}, 1);
    esp_lcd_panel_io_tx_param(io, 0x85, (uint8_t[]){0xFF}, 1);
    esp_lcd_panel_io_tx_param(io, 0x86, (uint8_t[]){0xFF}, 1);
    esp_lcd_panel_io_tx_param(io, 0x87, (uint8_t[]){0xFF}, 1);
    esp_lcd_panel_io_tx_param(io, 0x88, (uint8_t[]){0xA}, 1);
    esp_lcd_panel_io_tx_param(io, 0x89, (uint8_t[]){0x21}, 1);
    esp_lcd_panel_io_tx_param(io, 0x8A, (uint8_t[]){0x0}, 1);
    esp_lcd_panel_io_tx_param(io, 0x8B, (uint8_t[]){0x80}, 1);
    esp_lcd_panel_io_tx_param(io, 0x8C, (uint8_t[]){0x1}, 1);
    esp_lcd_panel_io_tx_param(io, 0x8D, (uint8_t[]){0x1}, 1);
    esp_lcd_panel_io_tx_param(io, 0x8E, (uint8_t[]){0xFF}, 1);
    esp_lcd_panel_io_tx_param(io, 0x8F, (uint8_t[]){0xFF}, 1);

    esp_lcd_panel_io_tx_param(io, 0xB6, (uint8_t[]){0x0, 0x20}, 2);

    esp_lcd_panel_io_tx_param(io, 0x36, (uint8_t[]){0x8}, 1);
    
    esp_lcd_panel_io_tx_param(io, 0x3A, (uint8_t[]){0x5}, 1);
    esp_lcd_panel_io_tx_param(io, 0x90, (uint8_t[]){0x8, 0x8, 0x8, 0x8}, 4);
    esp_lcd_panel_io_tx_param(io, 0xBD, (uint8_t[]){0x6}, 1);
    esp_lcd_panel_io_tx_param(io, 0xBC, (uint8_t[]){0x0}, 1);
    esp_lcd_panel_io_tx_param(io, 0xFF, (uint8_t[]){0x60, 0x1, 0x4}, 3);
    esp_lcd_panel_io_tx_param(io, 0xC3, (uint8_t[]){0x13}, 1);
    esp_lcd_panel_io_tx_param(io, 0xC4, (uint8_t[]){0x13}, 1);
    esp_lcd_panel_io_tx_param(io, 0xC9, (uint8_t[]){0x22}, 1);
    esp_lcd_panel_io_tx_param(io, 0xBE, (uint8_t[]){0x11}, 1);
    esp_lcd_panel_io_tx_param(io, 0xE1, (uint8_t[]){0x10, 0xE}, 2);
    esp_lcd_panel_io_tx_param(io, 0xDF, (uint8_t[]){0x21, 0xC, 0x2}, 3);
    esp_lcd_panel_io_tx_param(io, 0xF0, (uint8_t[]){
        0x45, 0x9, 0x8, 0x8, 0x26, 0x2A,
    }, 6);
    esp_lcd_panel_io_tx_param(io, 0xF1, (uint8_t[]){
        0x43, 0x70, 0x72, 0x36, 0x37, 0x6F
    }, 6);
    esp_lcd_panel_io_tx_param(io, 0xF2, (uint8_t[]){
        0x45, 0x9, 0x8, 0x8, 0x26, 0x2A,
    }, 6);
    esp_lcd_panel_io_tx_param(io, 0xF3, (uint8_t[]){
        0x43, 0x70, 0x72, 0x36, 0x37, 0x6F
    }, 6);

    esp_lcd_panel_io_tx_param(io, 0xED, (uint8_t[]){0x1B, 0xB}, 2);
    esp_lcd_panel_io_tx_param(io, 0xAE, (uint8_t[]){0x77}, 1);
    esp_lcd_panel_io_tx_param(io, 0xCD, (uint8_t[]){0x63}, 1);

    esp_lcd_panel_io_tx_param(io, 0x70, (uint8_t[]){
        0x7, 0x7, 0x4, 0xE, 0xF, 0x9, 0x7, 0x8, 0x3
    }, 9);

    esp_lcd_panel_io_tx_param(io, 0xE8, (uint8_t[]){0x34}, 1);
    esp_lcd_panel_io_tx_param(io, 0x62, (uint8_t[]){
        0x18, 0xD, 0x71, 0xED, 0x70, 0x70, 0x18, 0xF, 0x71, 0xEF, 0x70, 0x70
    }, 12);
    esp_lcd_panel_io_tx_param(io, 0x63, (uint8_t[]){
        0x18, 0x11, 0x71, 0xF1, 0x70, 0x70, 0x18, 0x13, 0x71, 0xF3, 0x70, 0x70
    }, 12);
    esp_lcd_panel_io_tx_param(io, 0x64, (uint8_t[]){
        0x28, 0x29, 0xF1, 0x1, 0xF1, 0x0, 0x7
    }, 7);
    esp_lcd_panel_io_tx_param(io, 0x66, (uint8_t[]){
        0x3C, 0x0, 0xCD, 0x67, 0x45, 0x45, 0x10, 0x0, 0x0, 0x0
    }, 10);
    esp_lcd_panel_io_tx_param(io, 0x67, (uint8_t[]){
        0x0, 0x3C, 0x0, 0x0, 0x0, 0x1, 0x54, 0x10, 0x32, 0x98
    }, 10);
    esp_lcd_panel_io_tx_param(io, 0x74, (uint8_t[]){
        0x10, 0x85, 0x80, 0x0, 0x0, 0x4E, 0x0
    }, 7);
    esp_lcd_panel_io_tx_param(io, 0x98, (uint8_t[]){0x3E, 0x7}, 2);
    // esp_lcd_panel_io_tx_param(io, LCD_CMD_TEON, NULL, 0);
    esp_lcd_panel_io_tx_param(io, LCD_CMD_INVON, NULL, 0);
    esp_lcd_panel_io_tx_param(io, LCD_CMD_SLPOUT, NULL, 0);
    vTaskDelay(pdMS_TO_TICKS(120));
    esp_lcd_panel_io_tx_param(io, LCD_CMD_DISPON, NULL, 0);
    vTaskDelay(pdMS_TO_TICKS(20));

    return ESP_OK;
}

static esp_err_t panel_gc9a01_draw_bitmap(esp_lcd_panel_t *panel, int x_start, int y_start, int x_end, int y_end, const void *color_data)
{
    gc9a01_panel_t *gc9a01 = __containerof(panel, gc9a01_panel_t, base);
    assert((x_start < x_end) && (y_start < y_end) && "start position must be smaller than end position");
    esp_lcd_panel_io_handle_t io = gc9a01->io;

    x_start += gc9a01->x_gap;
    x_end += gc9a01->x_gap;
    y_start += gc9a01->y_gap;
    y_end += gc9a01->y_gap;

    // define an area of frame memory where MCU can access
    esp_lcd_panel_io_tx_param(io, LCD_CMD_CASET, (uint8_t[]) {
        (x_start >> 8) & 0xFF,
        x_start & 0xFF,
        ((x_end - 1) >> 8) & 0xFF,
        (x_end - 1) & 0xFF,
    }, 4);
    esp_lcd_panel_io_tx_param(io, LCD_CMD_RASET, (uint8_t[]) {
        (y_start >> 8) & 0xFF,
        y_start & 0xFF,
        ((y_end - 1) >> 8) & 0xFF,
        (y_end - 1) & 0xFF,
    }, 4);
    // transfer frame buffer
    size_t len = (x_end - x_start) * (y_end - y_start) * gc9a01->fb_bits_per_pixel / 8;
    esp_lcd_panel_io_tx_color(io, LCD_CMD_RAMWR, color_data, len);

    return ESP_OK;
}

static esp_err_t panel_gc9a01_invert_color(esp_lcd_panel_t *panel, bool invert_color_data)
{
    gc9a01_panel_t *gc9a01 = __containerof(panel, gc9a01_panel_t, base);
    esp_lcd_panel_io_handle_t io = gc9a01->io;
    int command = 0;
    if (invert_color_data) {
        command = LCD_CMD_INVON;
    } else {
        command = LCD_CMD_INVOFF;
    }
    esp_lcd_panel_io_tx_param(io, command, NULL, 0);
    return ESP_OK;
}

static esp_err_t panel_gc9a01_mirror(esp_lcd_panel_t *panel, bool mirror_x, bool mirror_y)
{
    gc9a01_panel_t *gc9a01 = __containerof(panel, gc9a01_panel_t, base);
    esp_lcd_panel_io_handle_t io = gc9a01->io;
    if (mirror_x) {
        gc9a01->madctl_val |= LCD_CMD_MX_BIT;
    } else {
        gc9a01->madctl_val &= ~LCD_CMD_MX_BIT;
    }
    if (mirror_y) {
        gc9a01->madctl_val |= LCD_CMD_MY_BIT;
    } else {
        gc9a01->madctl_val &= ~LCD_CMD_MY_BIT;
    }
    esp_lcd_panel_io_tx_param(io, LCD_CMD_MADCTL, (uint8_t[]) {
        gc9a01->madctl_val
    }, 1);
    return ESP_OK;
}

static esp_err_t panel_gc9a01_swap_xy(esp_lcd_panel_t *panel, bool swap_axes)
{
    gc9a01_panel_t *gc9a01 = __containerof(panel, gc9a01_panel_t, base);
    esp_lcd_panel_io_handle_t io = gc9a01->io;
    if (swap_axes) {
        gc9a01->madctl_val |= LCD_CMD_MV_BIT;
    } else {
        gc9a01->madctl_val &= ~LCD_CMD_MV_BIT;
    }
    esp_lcd_panel_io_tx_param(io, LCD_CMD_MADCTL, (uint8_t[]) {
        gc9a01->madctl_val
    }, 1);
    return ESP_OK;
}

static esp_err_t panel_gc9a01_set_gap(esp_lcd_panel_t *panel, int x_gap, int y_gap)
{
    gc9a01_panel_t *gc9a01 = __containerof(panel, gc9a01_panel_t, base);
    gc9a01->x_gap = x_gap;
    gc9a01->y_gap = y_gap;
    return ESP_OK;
}

static esp_err_t panel_gc9a01_disp_on_off(esp_lcd_panel_t *panel, bool on_off)
{
    gc9a01_panel_t *gc9a01 = __containerof(panel, gc9a01_panel_t, base);
    esp_lcd_panel_io_handle_t io = gc9a01->io;
    int command = 0;
    if (on_off) {
        command = LCD_CMD_DISPON;
    } else {
        command = LCD_CMD_DISPOFF;
    }
    esp_lcd_panel_io_tx_param(io, command, NULL, 0);
    return ESP_OK;
}
