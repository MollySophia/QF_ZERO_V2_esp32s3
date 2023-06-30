#ifndef _BOARD_H
#define _BOARD_H

#define TXD_HC32_PIN_NUM 1
#define RXD_HC32_PIN_NUM 2
#define HC32_UART_NUM 1
#define HC32_UART_BAUDRATE 115200

#define KEY_INPUT_PIN_NUM 39
#define USB_INPUT_PIN_NUM 38

#define ESP_PRINT_PIN_NUM 46
#define PWM_MOTOR_PIN_NUM 47

void hc32_uart_init(void);
void board_init(void);

#endif