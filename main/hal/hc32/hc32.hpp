#pragma once
#include "hal_config.h"
#include "driver/uart.h"
#include "time.h"

#define DEBUG 1

namespace HC32 {
    static const char *TAG = "HC32";

    typedef enum
    {
        rec_head = 0,
        rec_name_len = 1,
        rec_name = 2,
        rec_data_len = 3,
        rec_data = 4,
        rec_crc = 5,
        pack_send_log = 65535,
    } trans_rec_sta_t;

    typedef struct {
        uint8_t dat_head[2];
        size_t data_count;
        uint8_t rec_buffer_name[24];
        uint8_t rec_buffer_data[64];
        size_t name_length;
        size_t data_length;
        size_t rec_p;
        uint16_t crc;
        trans_rec_sta_t rec_sta;
        uint8_t head_count;
    } packet_t;

    class HC32 {
        private:
            packet_t current_pack;
            tm rtc_time;
            inline void cmd_handler(packet_t *pack)
            {
            #if DEBUG
                ESP_LOGI(TAG, "cmd name: %s, data length: %i, data:", pack->rec_buffer_name, pack->data_length);
                for(int i = 0; i < pack->data_length; i++)
                    printf("%i%s", pack->rec_buffer_data[i], (i == pack->data_length - 1) ? "\n" : " ");
            #endif

                if(!strncmp((const char*)(pack->rec_buffer_name), "time", pack->name_length))
                {
                    rtc_time.tm_year = pack->rec_buffer_data[0] + 2000;
                    rtc_time.tm_mon = pack->rec_buffer_data[1];
                    rtc_time.tm_mday = pack->rec_buffer_data[2];
                    rtc_time.tm_wday = pack->rec_buffer_data[3];
                    rtc_time.tm_hour = pack->rec_buffer_data[4];
                    rtc_time.tm_min = pack->rec_buffer_data[5];
                    rtc_time.tm_sec = pack->rec_buffer_data[6];
                }
                // else if (/* condition */)
                // {
                //     /* code */
                // }
                
            }

            inline void _write_bytes(const char *str, size_t size)
            {
                uart_write_bytes(HC32_UART_NUM, str, size);
                uart_wait_tx_done(HC32_UART_NUM, portMAX_DELAY);
            }

            static void _rx_task(void *arg)
            {
                uint8_t tmp = 0;
                HC32 *obj = (HC32*)arg;
                vTaskDelay(pdMS_TO_TICKS(5));
                for (;;)
                {
                    int rxBytes = uart_read_bytes(HC32_UART_NUM, &tmp, 1, portMAX_DELAY);
                    if (rxBytes > 0)
                    {
                        // trans_packer_push_byte(hc32_trans_handle, tmp);
                        // ESP_LOGI(TAG, "Received char '%c'(0x%x) from HC32", (char)tmp, tmp);
                        obj->look_at_cmd_trans(tmp);
                    }
                }
            }

            void look_at_cmd_trans(uint8_t dat)
            {
                // 5 124 247 name_len name len_h len_l data crc_h crc_l
                ///////////////////////head///////////////////
                if (dat == 5)
                {
                    current_pack.dat_head[0] = dat;
                    current_pack.head_count = 0;
                }
                else if (dat == 124)
                {
                    if (current_pack.dat_head[0] == 5 && current_pack.head_count == 1)
                        current_pack.dat_head[1] = dat;
                    else
                        current_pack.dat_head[0] = 0;
                }
                else if (dat == 247)
                {
                    if (current_pack.head_count == 2 && current_pack.dat_head[1] == 124)
                    {
                        current_pack.rec_sta = rec_name_len;
                        memset(current_pack.rec_buffer_data, 0, sizeof(current_pack.rec_buffer_data));
                        memset(current_pack.rec_buffer_name, 0, sizeof(current_pack.rec_buffer_name));
                        return;
                    }
                    current_pack.dat_head[1] = 0;
                    current_pack.dat_head[0] = 0;
                }
                else
                {
                    current_pack.dat_head[1] = 0;
                    current_pack.dat_head[0] = 0;
                }
                current_pack.head_count++;
                ///////////////////////name len///////////////////////////////
                if (current_pack.rec_sta == rec_name_len)
                {
                    current_pack.name_length = dat;
                    if (current_pack.name_length > 24)
                    {
                        current_pack.rec_sta = rec_head;
                    }
                    else if (current_pack.name_length == 0)
                    {
                        current_pack.rec_sta = rec_data_len;
                        current_pack.data_count = 4;
                    }
                    else
                    {
                        current_pack.rec_sta = rec_name;
                        current_pack.rec_buffer_name[current_pack.name_length] = 0;
                        current_pack.rec_p = 0;
                    }
                }
                ///////////////////////name///////////////////////////////
                else if (current_pack.rec_sta == rec_name)
                {
                    current_pack.rec_buffer_name[current_pack.rec_p++] = dat;
                    if (current_pack.name_length == current_pack.rec_p)
                    {
                        current_pack.rec_sta = rec_data_len;
                        current_pack.data_count = 4;
                    }
                }
                ///////////////////////data len///////////////////////////////
                else if (current_pack.rec_sta == rec_data_len)
                {
                    if (current_pack.data_count == 5)
                        current_pack.data_length = dat << 8;
                    else if (current_pack.data_count == 6)
                    {
                        current_pack.data_length |= dat;

                        if (current_pack.data_length > 24)
                        {
                            current_pack.rec_sta = rec_head;
                        }
                        else if (current_pack.data_length == 0)
                        {
                            current_pack.rec_sta = rec_crc;
                            current_pack.data_count = 6;
                        }
                        else
                        {
                            current_pack.rec_sta = rec_data;
                            current_pack.rec_p = 0;
                        }
                    }
                }
                ///////////////////////data///////////////////////////////
                else if (current_pack.rec_sta == rec_data)
                {
                    current_pack.rec_buffer_data[current_pack.rec_p++] = dat;
                    if (current_pack.data_length == current_pack.rec_p)
                    {
                        current_pack.rec_sta = rec_crc;
                        current_pack.data_count = 6;
                    }
                }
                ///////////////////////crc///////////////////////////////
                else if (current_pack.rec_sta == rec_crc)
                {
                    if (current_pack.data_count == 7)
                        current_pack.crc = dat << 8;
                    else
                    {
                        uint16_t crc_tmp = 0;
                        size_t i = 0;
                        current_pack.crc |= dat;

                        for (i = 0; i < current_pack.name_length; i++)
                        {
                            crc_tmp += current_pack.rec_buffer_name[i];
                        }

                        for (i = 0; i < current_pack.data_length; i++)
                        {
                            crc_tmp += current_pack.rec_buffer_data[i];
                        }

                        if (current_pack.crc == crc_tmp) // crc ok
                        {
                            cmd_handler(&current_pack);
                        }
                        else // crc err
                        {
                        }
                        current_pack.rec_sta = rec_head;
                        return;
                    }
                }

                current_pack.data_count++;
                return;
            }

        public:
            inline void init()
            {
                gpio_config_t gpio_cfg = {
                    .pin_bit_mask = 1ULL << ESP_STA_PIN_NUM,
                    .mode = GPIO_MODE_INPUT,
                    .pull_up_en = GPIO_PULLUP_DISABLE,
                    .pull_down_en = GPIO_PULLDOWN_DISABLE,
                    .intr_type = GPIO_INTR_DISABLE,
                };

                gpio_config(&gpio_cfg);
                uint16_t tout = 0;
                while (gpio_get_level(ESP_STA_PIN_NUM) == 0 && tout++ < (1000 * 5))
                    vTaskDelay(pdMS_TO_TICKS(1));
                if(tout == 1000*5) ESP_LOGI(TAG, "Timed out waiting for HC32");
                ESP_LOGI(TAG, "Waited HC32 for %i ms", tout);

                uart_config_t uart_config = {
                    .baud_rate = HC32_UART_BAUDRATE,
                    .data_bits = UART_DATA_8_BITS,
                    .parity = UART_PARITY_DISABLE,
                    .stop_bits = UART_STOP_BITS_1,
                    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
                    .source_clk = UART_SCLK_APB,
                };

                ESP_ERROR_CHECK(uart_driver_install(HC32_UART_NUM, 256, 256, 0, NULL, 0));
                ESP_ERROR_CHECK(uart_param_config(HC32_UART_NUM, &uart_config));
                ESP_ERROR_CHECK(uart_set_pin(HC32_UART_NUM, TXD_HC32_PIN_NUM, RXD_HC32_PIN_NUM, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

                gpio_cfg.pin_bit_mask = (1ULL << ESP_STA_PIN_NUM) | (1ULL << ESP_PRINT_PIN_NUM);
                gpio_cfg.mode = GPIO_MODE_OUTPUT_OD;
                gpio_config(&gpio_cfg);
                gpio_set_level(ESP_STA_PIN_NUM, 0);

                xTaskCreate(_rx_task, "hc32_rx_task", 1024*4, this, configMAX_PRIORITIES, NULL);

                send_packet("get_time", NULL, 0);
            }

            inline void send_packet(const char *str, uint8_t* data, size_t size)
            {
                static uint8_t packet_head[3];
                uint8_t str_length = strlen(str);
                uint16_t crc = 0;

                packet_head[0] = 5;
                packet_head[1] = 124;
                packet_head[2] = 247;
                _write_bytes((const char*)packet_head, 3);

                _write_bytes((const char*)&str_length, 1);
                if(str_length)
                    _write_bytes(str, str_length);

                packet_head[0] = (size >> 8) & 0xff;
                packet_head[1] = size & 0xff;
                _write_bytes((const char*)packet_head, 2);

                if(size)
                    _write_bytes((const char*)data, size);
                
                for(int i = 0; i < str_length; i++)
                    crc += str[i];
                
                for(int i = 0; i < size; i++)
                    crc += data[i];

                packet_head[0] = (crc >> 8) & 0xff;
                packet_head[1] = crc & 0xff;
                _write_bytes((const char*)packet_head, 2);
            }

            inline void get_time(tm *time)
            {
                send_packet("get_time", NULL, 0);
                vTaskDelay(pdMS_TO_TICKS(20));
                if(time != NULL)
                    memcpy(time, &rtc_time, sizeof(tm));
            }

            inline void set_time(tm *time)
            {
                if(time == NULL)
                    return;
                uint8_t tmp[6] = {
                    (uint8_t)(time->tm_year - 2000),
                    (uint8_t)time->tm_mon,
                    (uint8_t)time->tm_wday,
                    (uint8_t)time->tm_hour,
                    (uint8_t)time->tm_min,
                    (uint8_t)time->tm_sec
                };

                send_packet("set_time", tmp, sizeof(tmp));
            }
    };
}