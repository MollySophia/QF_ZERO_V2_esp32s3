/**
 * @file hal.h
 * @author Forairaaaaa
 * @brief 
 * @version 0.1
 * @date 2023-05-20
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#pragma once
#include "button/Button.h"
#include "buzzer/hal_buzzer.hpp"
#include "disp/hal_disp.hpp"
#include "lvgl/hal_lvgl.hpp"
#include "power/hal_power.hpp"
#include "rtc/hal_rtc.hpp"
#include "tp/hal_tp.hpp"
#include "hal_config.h"

class HAL {
    private:
        bool _isSleeping ;


    public:
        HAL() : _isSleeping(false) {}
        ~HAL() = default;

        
        /* Sleep flag */
        inline void isSleeping(bool sleep) { _isSleeping = sleep; }
        inline bool isSleeping(void) { return _isSleeping; } 


        /* Display */
        GC9A01 disp;

        /* Touch pad */
        CST816S::TP_CST816S tp;

        /* Fuel gauge*/
        CW2015::CW2015 pmu;

        /* RTC PCF8563 */
        // PCF8563::PCF8563 rtc;

        /* SD card */
        // SD_CARD::SD_Card sd;

        /* Buttons */
        Button btn = Button(KEY_INPUT_PIN_NUM);
        /* Buzzer */
        // BUZZER::BUZZER buzz;

        /* IMU BMM270 + BMM150 */
        // BMI270::BMI270 imu;

        /* Lvgl */
        LVGL::LVGL lvgl;


        /**
         * @brief Hal init 
         * 
         */
        void init();


        /**
         * @brief Update hal
         * 
         */
        void update();


        /**
         * @brief Normal or USB MSC mode 
         * 
         */
        void checkBootMode();


};
