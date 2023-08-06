/**
 * @file hal.cpp
 * @author Forairaaaaa
 * @brief 
 * @version 0.1
 * @date 2023-05-20
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include "hal.h"
#include "hal_config.h"
#include "driver/i2c.h"
#include <esp_sleep.h>


void HAL::init()
{
    const char *TAG = "HAL_INIT";
    hc32.init();
    /* Display */
    disp.init();

    /* Touch pad and I2C port 0 (default) */
    auto cfg = tp.config();
    cfg.pull_up_en = false;
    tp.config(cfg);
    tp.init(TP_PIN_NUM_SDA, TP_PIN_NUM_SCL, 0, TP_PIN_NUM_INT, true, 400000);

    /* I2C port */
    // esp_err_t ret;
    // i2c_config_t conf;
    // memset(&conf, 0, sizeof(i2c_config_t));
    // conf.mode = I2C_MODE_MASTER;
    // conf.sda_io_num = I2C_SENSORS_SDA_PIN_NUM;
    // conf.scl_io_num = I2C_SENSORS_SCL_PIN_NUM;
    // conf.master.clk_speed = 300*1000;
    // conf.sda_pullup_en = GPIO_PULLUP_DISABLE;
    // conf.scl_pullup_en = GPIO_PULLUP_DISABLE;
    // conf.clk_flags = I2C_SCLK_SRC_FLAG_FOR_NOMAL;
    // ret = i2c_param_config(I2C_NUM_1, &conf);
    // if (ret != ESP_OK) {
    //     ESP_LOGE(TAG, "I2C1 config failed");
    // }

    // /* Install driver */
    // ret = i2c_driver_install(I2C_NUM_1, I2C_MODE_MASTER, 0, 0, 0);
    // if (ret != ESP_OK) {
    //     ESP_LOGE(TAG, "I2C1 driver install failed");
    // }

    /* CW2015 Fuel gauge */
    // pmu.init(I2C_SENSORS_SDA_PIN_NUM, I2C_SENSORS_SCL_PIN_NUM, -1);

    /* Buttons */
    btn.begin();

    /* Lvgl */
    lvgl.init(&disp, &tp);

    esp_sleep_enable_ext0_wakeup(RXD_HC32_PIN_NUM, 0);
}


void HAL::update()
{
    lvgl.update();
}

void HAL::checkBootMode()
{

}