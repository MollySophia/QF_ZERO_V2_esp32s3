#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_gc9a01.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_commands.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "hal_config.h"
#include <cstring>

class GC9A01 {
    esp_lcd_panel_handle_t _panel_handle;
    esp_lcd_panel_io_handle_t _panel_io_handle;
    esp_lcd_i80_bus_handle_t _bus_handle;
    uint32_t _width;
    uint32_t _height;
    ledc_channel_t _bl_ledc_channel;

    public:
        uint32_t width() {return _width;}
        uint32_t height() {return _height;}
        GC9A01(void)
        {
            _height = 240;
            _width = 240;
            _bl_ledc_channel = LEDC_CHANNEL_0;

            {
                esp_lcd_i80_bus_config_t bus_config = {
                    .dc_gpio_num = LCD_PIN_NUM_DC,
                    .wr_gpio_num = LCD_PIN_NUM_CLK,
                    .clk_src = LCD_CLK_SRC_DEFAULT,
                    .data_gpio_nums = {
                        LCD_PIN_NUM_DATA0,
                        LCD_PIN_NUM_DATA1,
                        LCD_PIN_NUM_DATA2,
                        LCD_PIN_NUM_DATA3,
                        LCD_PIN_NUM_DATA4,
                        LCD_PIN_NUM_DATA5,
                        LCD_PIN_NUM_DATA6,
                        LCD_PIN_NUM_DATA7,
                    },
                    .bus_width = 8,
                    .max_transfer_bytes = _height * _width * sizeof(uint16_t), // transfer 100 lines of pixels (assume pixel is RGB565) at most in one transaction
                };
                ESP_ERROR_CHECK(esp_lcd_new_i80_bus(&bus_config, &_bus_handle));
            }

            {
                esp_lcd_panel_io_i80_config_t io_config = {
                    .cs_gpio_num = LCD_PIN_NUM_CS,
                    .pclk_hz = 8000000,
                    .trans_queue_depth = 10,
                    .lcd_cmd_bits = 8,
                    .lcd_param_bits = 8,
                    .dc_levels = {
                        .dc_idle_level = 0,
                        .dc_cmd_level = 0,
                        .dc_dummy_level = 0,
                        .dc_data_level = 1,
                    },
                };
                ESP_ERROR_CHECK(esp_lcd_new_panel_io_i80(_bus_handle, &io_config, &_panel_io_handle));
            }

            {
                esp_lcd_panel_dev_config_t panel_config = {
                    .reset_gpio_num = LCD_PIN_NUM_RST,
                    .rgb_endian = LCD_RGB_ENDIAN_RGB,
                    .bits_per_pixel = 16,
                };
                ESP_ERROR_CHECK(esp_lcd_new_panel_gc9a01(_panel_io_handle, &panel_config, &_panel_handle));
            }

            {
                ledc_timer_config_t ledc_timer = {
                    .speed_mode       = LEDC_LOW_SPEED_MODE,
                    .duty_resolution  = LEDC_TIMER_8_BIT,
                    .timer_num        = LEDC_TIMER_0,
                    .freq_hz          = 10000,
                    .clk_cfg          = LEDC_AUTO_CLK
                };
                ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

                ledc_channel_config_t ledc_channel = {
                    .gpio_num       = LCD_PIN_NUM_PWM_BL,
                    .speed_mode     = LEDC_LOW_SPEED_MODE,
                    .channel        = _bl_ledc_channel,
                    .intr_type      = LEDC_INTR_DISABLE,
                    .timer_sel      = LEDC_TIMER_0,
                    .duty           = 0,
                    .hpoint         = 0
                };
                ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
            }

            gpio_config_t lcd_en_gpio_config = {
                .pin_bit_mask = 1ULL << LCD_PIN_NUM_EN,
                .mode = GPIO_MODE_OUTPUT,
            };
            ESP_ERROR_CHECK(gpio_config(&lcd_en_gpio_config));
        }

        inline bool init(void) {
            gpio_set_level(LCD_PIN_NUM_EN, 0);
            setBrightness(0);
            esp_lcd_panel_reset(_panel_handle);
            esp_lcd_panel_init(_panel_handle);

            ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(_panel_handle, true));
            return true;
        }

        void setBrightness(uint8_t brightness) {
            ledc_set_duty(LEDC_LOW_SPEED_MODE, _bl_ledc_channel, brightness);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, _bl_ledc_channel);
        }

        void drawBitmap(int x_start, int y_start, int x_end, int y_end, const void *color_data) {
            esp_lcd_panel_draw_bitmap(_panel_handle, x_start, y_start, x_end, y_end, color_data);
        }
};