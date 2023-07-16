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


void HAL::init()
{
    const char *TAG = "HAL_INIT";
    /* Display */
    disp.init();
    disp.setBrightness(200);


    /* Touch pad and I2C port 0 (default) */
    auto cfg = tp.config();
    cfg.pull_up_en = false;
    tp.config(cfg);
    tp.init(TP_PIN_NUM_SDA, TP_PIN_NUM_SCL, 0, TP_PIN_NUM_INT, true, 400000);

    /* I2C port */
    esp_err_t ret;
    i2c_config_t conf;
    memset(&conf, 0, sizeof(i2c_config_t));
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_SENSORS_SDA_PIN_NUM;
    conf.scl_io_num = I2C_SENSORS_SCL_PIN_NUM;
    conf.master.clk_speed = 400*1000;
    conf.sda_pullup_en = GPIO_PULLUP_DISABLE;
    conf.scl_pullup_en = GPIO_PULLUP_DISABLE;
    conf.clk_flags = I2C_SCLK_SRC_FLAG_FOR_NOMAL;
    ret = i2c_param_config(I2C_NUM_1, &conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C1 config failed");
    }

    /* Install driver */
    ret = i2c_driver_install(I2C_NUM_1, I2C_MODE_MASTER, 0, 0, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C1 driver install failed");
    }


    /* CW2015 Fuel gauge */
    pmu.init(I2C_SENSORS_SDA_PIN_NUM, I2C_SENSORS_SDA_PIN_NUM, -1);


    /* Buttons */
    btn.begin();


    /* Once button and power setup, check boot mode */
    // checkBootMode();


    /* RTC PCF8563 */
    // rtc.init(HAL_PIN_I2C_SDA, HAL_PIN_I2C_SCL, HAL_PIN_RTC_INTR);


    /* Buzzer */
    // buzz.init(HAL_PIN_BUZZER);


    /* SD card */
    // sd.init();


    /* Lvgl */
    lvgl.init(&disp, &tp);


    /* IMU BMI270 + BMM150 */
    // imu.init();
    // /* Setup wrist wear wakeup interrupt */
    // imu.setWristWearWakeup();
    // /* Enable step counter */
    // imu.enableStepCounter();


}


void HAL::update()
{
    lvgl.update();
}


// const std::string disk_ascii = R"(
//    ****     ### *
//  ******     ### ****
//  ******         *****
//  ********************
//  ****/,,,,,,,,,,,/***
//  ****             ***
//  ****             ***
//  ****             ***
// )";


void HAL::checkBootMode()
{
    /* Press button B while power on to enter USB MSC mode */
    // if (!btnB.read()) {
    //     vTaskDelay(pdMS_TO_TICKS(20));
    //     if (!btnB.read()) {

    //         disp.fillScreen(TFT_BLACK);
    //         disp.setTextSize(3);
    //         disp.setCursor(0, 50);
    //         disp.printf(" :)\n Release Key\n To Enter\n USB MSC Mode\n");

    //         /* Wait release */
    //         while (!btnB.read()) {
    //             vTaskDelay(pdMS_TO_TICKS(10));
    //         }

    //         disp.fillScreen(TFT_BLACK);
    //         disp.setTextSize(3);
    //         disp.setCursor(0, 50);
    //         // disp.printf(" USB MSC Mode\n\n\n\n\n\n\n\n\n\n Reboot     ->");
    //         disp.printf(" [ USB MSC Mode ]\n");

    //         disp.setTextSize(2);
    //         disp.printf("\n\n\n%s\n\n\n\n Press to quit            ->", disk_ascii.c_str());


    //         /* Enable usb msc */
    //         hal_enter_usb_msc_mode();


    //         /* Simply restart make usb not vailable, dont know why */
    //         pmu.powerOff();
    //         while (1) {
    //             vTaskDelay(1000);
    //         }

    //     }
    // }

}