#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"

#include "board.h"

static const char *TAG = "board";

void hc32_uart_init() {
    const uart_port_t uart_num = HC32_UART_NUM;
    uart_config_t uart_config = {
        .baud_rate = HC32_UART_BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };

    ESP_ERROR_CHECK(uart_driver_install(HC32_UART_NUM, 1024 * 2, 0, 0, NULL, ESP_INTR_FLAG_IRAM));
    ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(HC32_UART_NUM, TXD_HC32_PIN_NUM, RXD_HC32_PIN_NUM, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
}

void board_init() {
    hc32_uart_init();
}