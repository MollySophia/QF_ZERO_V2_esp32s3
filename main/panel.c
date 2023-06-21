#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_gc9a01.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_commands.h"
#include "esp_lcd_touch.h"
#include "esp_lcd_touch_cst816s.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "panel.h"

static const char *TAG = "panel";

esp_err_t display_init(esp_lcd_panel_handle_t *panel_handle, esp_lcd_panel_io_handle_t *io_handle) {
    esp_lcd_i80_bus_handle_t i80_bus = NULL;
    esp_lcd_i80_bus_config_t bus_config = {
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .dc_gpio_num = LCD_PIN_NUM_DC,
        .wr_gpio_num = LCD_PIN_NUM_CLK,
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
        .max_transfer_bytes = LCD_H_RES * LCD_V_RES * sizeof(uint16_t), // transfer 100 lines of pixels (assume pixel is RGB565) at most in one transaction
    };
    ESP_ERROR_CHECK(esp_lcd_new_i80_bus(&bus_config, &i80_bus));

    assert(io_handle);
    esp_lcd_panel_io_i80_config_t io_config = {
        .cs_gpio_num = LCD_PIN_NUM_CS,
        .pclk_hz = 8000000,
        .trans_queue_depth = 10,
        .dc_levels = {
            .dc_idle_level = 0,
            .dc_cmd_level = 0,
            .dc_dummy_level = 0,
            .dc_data_level = 1,
        },
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i80(i80_bus, &io_config, io_handle));

    assert(panel_handle);
    ESP_LOGI(TAG, "Install LCD driver of gc9a01");
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = LCD_PIN_NUM_RST,
        .rgb_endian = LCD_RGB_ENDIAN_RGB,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_gc9a01(*io_handle, &panel_config, panel_handle));

    esp_lcd_panel_reset(*panel_handle);
    esp_lcd_panel_init(*panel_handle);

    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(*panel_handle, true));

    return ESP_OK;
}

esp_err_t touch_init(esp_lcd_touch_handle_t *touch_handle, esp_lcd_panel_io_handle_t *io_handle) {
    i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = TP_PIN_NUM_SDA,
        .scl_io_num = TP_PIN_NUM_SCL,
        .sda_pullup_en = GPIO_PULLUP_DISABLE,
        .scl_pullup_en = GPIO_PULLUP_DISABLE,
        .master.clk_speed = 400*1000
    };
    ESP_ERROR_CHECK(i2c_param_config(0, &i2c_conf));
    ESP_ERROR_CHECK(i2c_driver_install(0, I2C_MODE_MASTER, 0, 0, 0));

    esp_lcd_panel_io_i2c_config_t io_config = 
        ESP_LCD_TOUCH_IO_I2C_CST816S_CONFIG();
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)0, &io_config, io_handle));

    esp_lcd_touch_config_t tp_cfg = {
        .x_max = LCD_H_RES,
        .y_max = LCD_V_RES,
        .rst_gpio_num = -1,
        .int_gpio_num = TP_PIN_NUM_INT,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = 0,
            .mirror_x = 1,
            .mirror_y = 0,
        },
    };

    assert(touch_handle);
    assert(io_handle);
    return esp_lcd_touch_new_i2c_cst816s(*io_handle, &tp_cfg, touch_handle);
}
