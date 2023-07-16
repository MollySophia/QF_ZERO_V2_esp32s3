#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_err.h"
#include "esp_log.h"
#include "mooncake.h"
#include "hardware_manager.h"

#include "main_ui.h"

HM::Hardware_Manager hardware_manager;
extern MOONCAKE::Mooncake mooncake;

#ifdef __cplusplus
extern "C" {
#endif

static void ui_task(void *parameter) {
    while(1) {
        hardware_manager.update();
        main_ui_update();
        vTaskDelay(pdMS_TO_TICKS(2));
    }
}

static void lvgl_port_tick_increment(void *arg)
{
    /* Tell LVGL how many milliseconds have elapsed */
    lv_tick_inc(5);
}

void app_main(void)
{
    hardware_manager.init();
    main_ui_create();

    hardware_manager.setMooncake(&mooncake);

    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &lvgl_port_tick_increment,
        .name = "LVGL tick",
    };
    esp_timer_handle_t tick_timer;
    esp_timer_create(&lvgl_tick_timer_args, &tick_timer);
    esp_timer_start_periodic(tick_timer, 5 * 1000);

    xTaskCreatePinnedToCore(ui_task, "UI_TASK", 4096, NULL, 5, NULL, 0);
}

#ifdef __cplusplus
}
#endif