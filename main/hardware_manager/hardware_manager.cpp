/**
 * @file hardware_manager.cpp
 * @author Forairaaaaa
 * @brief 
 * @version 0.1
 * @date 2023-05-22
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include "hardware_manager.h"
#include "../hal/hal_config.h"
#include <esp_log.h>
#include <esp_sleep.h>
#include <esp_timer.h>


#define delay(ms) vTaskDelay(pdMS_TO_TICKS(ms))


namespace HM {


    static const char* TAG = "HM";


    void Hardware_Manager::_update_rtc_time()
    {
        /* If time just set */
        if (*_rtc_data.time_just_set_ptr) {
            
            /* Reset flag */
            *_rtc_data.time_just_set_ptr = false;

            printf("Setting time\n");

            /* Write into RTC data buffer */
            _rtc_data.rtc_time.tm_hour = _rtc_data.time_ptr->hour;
            _rtc_data.rtc_time.tm_min = _rtc_data.time_ptr->min;
            _rtc_data.rtc_time.tm_sec = _rtc_data.time_ptr->sec;
            _rtc_data.rtc_time.tm_year = _rtc_data.time_ptr->year + 1900;
            _rtc_data.rtc_time.tm_mon = _rtc_data.time_ptr->mon;
            _rtc_data.rtc_time.tm_mday = _rtc_data.time_ptr->mday;
            _rtc_data.rtc_time.tm_wday = _rtc_data.time_ptr->wday;

            /* Set RTC time */
            // rtc.setTime(_rtc_data.rtc_time);
            time_t t = mktime(&_rtc_data.rtc_time);
            struct timeval time_set = {.tv_sec = t};
            settimeofday(&time_set, NULL);

        }
        else {

            /* Read RTC */
            time_t time_seconds = time(0);
            setenv("TZ", "CST-8", 1);
            tzset();

            localtime_r(&time_seconds, &_rtc_data.rtc_time);
            // rtc.getTime(_rtc_data.rtc_time);
            // printf("%02d:%02d:%02d %d-%d-%d-%d\n",
            //     _rtc_data.rtc_time.tm_hour, _rtc_data.rtc_time.tm_min, _rtc_data.rtc_time.tm_sec,
            //     _rtc_data.rtc_time.tm_year, _rtc_data.rtc_time.tm_mon + 1, _rtc_data.rtc_time.tm_mday, _rtc_data.rtc_time.tm_wday);

            /* Write into database */
            _rtc_data.time_ptr->hour = _rtc_data.rtc_time.tm_hour;
            _rtc_data.time_ptr->min = _rtc_data.rtc_time.tm_min;
            _rtc_data.time_ptr->sec = _rtc_data.rtc_time.tm_sec;
            _rtc_data.time_ptr->year = _rtc_data.rtc_time.tm_year - 1900;
            _rtc_data.time_ptr->mon = _rtc_data.rtc_time.tm_mon;
            _rtc_data.time_ptr->mday = _rtc_data.rtc_time.tm_mday;
            _rtc_data.time_ptr->wday = _rtc_data.rtc_time.tm_wday;

        }

        /* Update power infos also */
        _update_power_infos();
    }


    void Hardware_Manager::_update_imu_data()
    {
        /* Read IMU */
        // *_imu_data.steps = imu.getSteps();
    }


    void Hardware_Manager::_update_power_infos()
    {
        *_power_infos.battery_level_ptr = pmu.batteryLevel();
        *_power_infos.battery_is_charging_ptr = pmu.isCharging();
    }


    void Hardware_Manager::_update_go_sleep()
    {
        /* Check lvgl inactive time */
        if (lv_disp_get_inactive_time(NULL) > _power_manager.auto_sleep_time) {
            _power_manager.power_mode = mode_sleeping;
        }
    
    }


    void Hardware_Manager::_update_power_mode()
    {
        if (_power_manager.power_mode == mode_sleeping) {
            
            ESP_LOGI(TAG, "going sleep...");

            /* Close display */
            disp.setBrightness(0);
            
            /* Setup wakeup pins */
            /* Key */
            gpio_reset_pin(KEY_INPUT_PIN_NUM);
            gpio_set_direction(KEY_INPUT_PIN_NUM, GPIO_MODE_INPUT);
            gpio_sleep_set_pull_mode(KEY_INPUT_PIN_NUM, GPIO_PULLUP_ONLY);
            gpio_wakeup_enable(KEY_INPUT_PIN_NUM, GPIO_INTR_LOW_LEVEL);
            /* USB */
            gpio_reset_pin(USB_INPUT_PIN_NUM);
            gpio_set_direction(USB_INPUT_PIN_NUM, GPIO_MODE_INPUT);
            gpio_sleep_set_pull_mode(USB_INPUT_PIN_NUM, GPIO_PULLDOWN_ONLY);
            gpio_wakeup_enable(USB_INPUT_PIN_NUM, GPIO_INTR_POSEDGE);
            /* Touch */
            // gpio_reset_pin(TP_PIN_NUM_INT);
            // gpio_set_direction(TP_PIN_NUM_INT, GPIO_MODE_INPUT);
            // gpio_sleep_set_pull_mode(TP_PIN_NUM_INT, GPIO_FLOATING);
            // gpio_wakeup_enable(TP_PIN_NUM_INT, GPIO_INTR_LOW_LEVEL);

            esp_sleep_enable_gpio_wakeup();

            /* Go to sleep :) */
            esp_light_sleep_start();

            /* ---------------------------------------------------------------- */

            /* Wake up o.O */
            _power_manager.power_mode = mode_normal;

            /* Update data at once */
            _update_rtc_time();
            _update_imu_data();
            /* Clear key pwr */
            // pmu.isKeyPressed();

            /* Tell Mooncake */
            *_system_data.just_wake_up_ptr = true;

            /* Restart display */
            disp.init();
            disp.setBrightness(0);

            /* Reset lvgl inactive time */
            lv_disp_trig_activity(NULL);

            /* Update a little bit before display on */
            getMooncake()->update();

            /* Refresh full screen */
            lv_obj_invalidate(lv_scr_act());

            /* Display on */
            disp.setBrightness(200);
        }
    }


    void Hardware_Manager::_update_key_data()
    {
        /* Key Home or Power */
        if (btn.pressed()) {
            *_key_data.key_home_ptr = true;
            *_key_data.key_pwr_ptr = true;
        }
    }


    void Hardware_Manager::setMooncake(MOONCAKE::Mooncake* mooncake)
    {
        if (mooncake == nullptr) {
            ESP_LOGE(TAG, "empty pointer");
            return;
        }
        _mooncake = mooncake;


        /* Get data's pointer in database */

        /* Time */
        _rtc_data.time_ptr = (MOONCAKE::DataTime_t*)getDatabase()->Get(MC_TIME)->addr;
        _rtc_data.time_just_set_ptr = (bool*)getDatabase()->Get(MC_TIME_JSUT_SET)->addr;

        /* Power infos */
        _power_infos.battery_level_ptr = (uint8_t*)getDatabase()->Get(MC_BATTERY_LEVEL)->addr;
        _power_infos.battery_is_charging_ptr = (bool*)getDatabase()->Get(MC_BATTERY_IS_CHARGING)->addr;

        /* IMU */
        // _imu_data.steps = (uint32_t*)getDatabase()->Get(MC_STEPS)->addr;

        /* System data */
        _system_data.just_wake_up_ptr = (bool*)getDatabase()->Get(MC_JUST_WAKEUP)->addr;

        /* Keys */
        _key_data.key_home_ptr = (bool*)getDatabase()->Get(MC_KEY_HOME)->addr;
        _key_data.key_pwr_ptr = (bool*)getDatabase()->Get(MC_KEY_POWER)->addr;
        _key_data.key_up_ptr = (bool*)getDatabase()->Get(MC_KEY_UP)->addr;
        _key_data.key_down_ptr = (bool*)getDatabase()->Get(MC_KEY_DOWN)->addr;
    }


    void Hardware_Manager::init()
    {
        HAL::init();
    }

    
    void Hardware_Manager::update()
    {
        /* Update RTC */
        if ((esp_timer_get_time() - _rtc_data.update_count) > _rtc_data.update_interval) {
            _update_rtc_time();
            _rtc_data.update_count = esp_timer_get_time();
        }
        
        /* Update IMU */
        // if ((esp_timer_get_time() - _imu_data.update_count) > _imu_data.update_interval) {
        //     _update_imu_data();
        //     _imu_data.update_count = esp_timer_get_time();
        // }

        /* Update keys */
        if ((esp_timer_get_time() - _key_data.update_count) > _key_data.update_interval) {
            _update_key_data();
            _key_data.update_count = esp_timer_get_time();
        }


        /* Update power control */
        _update_go_sleep();
        _update_power_mode();

        /* HAL update */
        HAL::update();
    }


    

}
