#pragma once
#include <esp_log.h>
#include <lvgl.h>
#include "../disp/hal_disp.hpp"
#include "../tp/hal_tp.hpp"

#include "lv_demos.h"


namespace LVGL {


    static const char* TAG = "LVGL";

    static GC9A01* _disp;
    static CST816S::TP_CST816S* _tp;
    static CST816S::TouchPoint_t _tpp;


    class LVGL {
        private:
            /*Flush the content of the internal buffer the specific area on the display
            *You can use DMA or any hardware acceleration to do this operation in the background but
            *'lv_disp_flush_ready()' has to be called when finished.*/
            static void _disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
            {
                _disp->drawBitmap(area->x1, area->y1, area->x2+1, area->y2+1, color_p);

                /*IMPORTANT!!!
                *Inform the graphics library that you are ready with the flushing*/
                lv_disp_flush_ready(disp_drv);
            }


            /*Will be called by the library to read the touchpad*/
            static void _touchpad_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
            {
                _tp->getTouchRaw(_tpp);
                if(_tpp.x != -1) {
                    // printf("%d %d\n", _tpp.x, _tpp.y);
                    data->point.x = _tpp.x;
                    data->point.y = _tpp.y;
                    data->state = LV_INDEV_STATE_PR;
                }
                else {
                    data->state = LV_INDEV_STATE_REL;
                }
            }


            inline void _lv_port_disp_init()
            {
                static lv_disp_draw_buf_t draw_buf_dsc_3;
                // lv_color_t * buf_3_1 = (lv_color_t *)heap_caps_malloc(_disp->width() * _disp->height() * sizeof(lv_color_t), MALLOC_CAP_DEFAULT);
                // lv_color_t * buf_3_2 = (lv_color_t *)heap_caps_malloc(_disp->width() * _disp->height() * sizeof(lv_color_t), MALLOC_CAP_DEFAULT);
                lv_color_t * buf_3_1 = (lv_color_t *)malloc(_disp->width() * _disp->height() * sizeof(lv_color_t));
                lv_color_t * buf_3_2 = (lv_color_t *)malloc(_disp->width() * _disp->height() * sizeof(lv_color_t));

                /* If failed */
                if ((buf_3_1 == NULL) || (buf_3_2 == NULL)) {
                    ESP_LOGE(TAG, "malloc buffer from PSRAM fialed");
                    while (1);
                } else {
                    ESP_LOGI(TAG, "malloc buffer from PSRAM successful");
                    ESP_LOGI(TAG, "free PSRAM: %d\r\n", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
                }

                lv_disp_draw_buf_init(&draw_buf_dsc_3, buf_3_1, buf_3_2,
                                    _disp->width() * _disp->height());   /*Initialize the display buffer*/

                /*-----------------------------------
                * Register the display in LVGL
                *----------------------------------*/

                static lv_disp_drv_t disp_drv;                         /*Descriptor of a display driver*/
                lv_disp_drv_init(&disp_drv);                    /*Basic initialization*/

                /*Set up the functions to access to your display*/

                /*Set the resolution of the display*/
                disp_drv.hor_res = _disp->width();
                disp_drv.ver_res = _disp->height();

                /*Used to copy the buffer's content to the display*/
                disp_drv.flush_cb = _disp_flush;

                /*Set a display buffer*/
                disp_drv.draw_buf = &draw_buf_dsc_3;

                /*Required for Example 3)*/
                // disp_drv.full_refresh = 1;

                /* Fill a memory array with a color if you have GPU.
                * Note that, in lv_conf.h you can enable GPUs that has built-in support in LVGL.
                * But if you have a different GPU you can use with this callback.*/
                //disp_drv.gpu_fill_cb = gpu_fill;

                /*Finally register the driver*/
                lv_disp_drv_register(&disp_drv);
            }


            inline void _lv_port_indev_init()
            {
                /**
                 * Here you will find example implementation of input devices supported by LittelvGL:
                 *  - Touchpad
                 *  - Mouse (with cursor support)
                 *  - Keypad (supports GUI usage only with key)
                 *  - Encoder (supports GUI usage only with: left, right, push)
                 *  - Button (external buttons to press points on the screen)
                 *
                 *  The `..._read()` function are only examples.
                 *  You should shape them according to your hardware
                 */
                static lv_indev_drv_t indev_drv;

                lv_group_t *g = lv_group_create();
                lv_group_set_default(g);

                /*------------------
                * Touchpad
                * -----------------*/
                /*Register a touchpad input device*/
                lv_indev_drv_init(&indev_drv);
                indev_drv.type = LV_INDEV_TYPE_POINTER;
                indev_drv.read_cb = _touchpad_read;
                // indev_touchpad = lv_indev_drv_register(&indev_drv);
                lv_indev_drv_register(&indev_drv);
            }


        public:
            /**
             * @brief Lvgl init
             * 
             * @param disp 
             * @param tp 
             */
            inline void init(GC9A01* disp, CST816S::TP_CST816S* tp)
            { 
                _disp = disp;
                _tp = tp;
                
                lv_init();
                _lv_port_disp_init();
                _lv_port_indev_init();
            }


            /**
             * @brief Update lvgl timer 
             * 
             */
            inline void update() { lv_timer_handler(); }
    };

}
