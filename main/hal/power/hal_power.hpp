#pragma once
#include <driver/i2c.h>
#include <esp_log.h>

#define REG_VERSION 0x0
#define REG_VCELL 0x2
#define REG_SOC 0x4
#define REG_RRT_ALERT 0x6
#define REG_CONFIG 0x8
#define REG_MODE 0xA
#define REG_BATINFO 0x10

namespace CW2015 {
    static const char* TAG = "CW2015";
    static const uint8_t cw_bat_config_info[] = {
                0X15, 0X7E, 0X7C, 0X5C, 0X64, 0X6A, 0X65, 0X5C, 0X55, 0X53, 0X56, 0X61, 0X6F, 0X66, 0X50, 0X48,
                0X43, 0X42, 0X40, 0X43, 0X4B, 0X5F, 0X75, 0X7D, 0X52, 0X44, 0X07, 0XAE, 0X11, 0X22, 0X40, 0X56,
                0X6C, 0X7C, 0X85, 0X86, 0X3D, 0X19, 0X8D, 0X1B, 0X06, 0X34, 0X46, 0X79, 0X8D, 0X90, 0X90, 0X46,
                0X67, 0X80, 0X97, 0XAF, 0X80, 0X9F, 0XAE, 0XCB, 0X2F, 0X00, 0X64, 0XA5, 0XB5, 0X11, 0XD0, 0X11};

    enum
    {
        MODE_SLEEP,
        MODE_NORMAL,
        MODE_QUICK_START,
        MODE_RESTART,
        CONFIG_UPDATE_FLG,
        ATHD_DEFAULT
    };

    static uint8_t reg_buffer[] = {(0x3 << 6), (0x0 << 6), (0x3 << 4), (0xf << 0), (0x1 << 1), (3 << 3)};

    struct Config_t {
        int pin_scl     = -1;
        int pin_sda     = -1;
        int pin_int     = -1;
        i2c_port_t i2c_port = I2C_NUM_1;

        uint8_t dev_addr = 0xC4;
    };


    class CW2015 {
        private:
            Config_t _cfg;
            uint8_t _data_buffer[2];
            uint8_t _regs[7];
            int voltage;
            int soc;
            int rtt;
            int low_soc;

            inline esp_err_t _writeReg(uint8_t reg, uint8_t data)
            {
                _data_buffer[0] = reg;
                _data_buffer[1] = data; 
                return i2c_master_write_to_device(_cfg.i2c_port, _cfg.dev_addr, _data_buffer, 2, portMAX_DELAY);
            }

            inline esp_err_t _writeReg(uint8_t reg, uint8_t *data, uint size)
            {
                uint8_t *buffer = new uint8_t[size + 1];
                buffer[0] = reg;
                memcpy(buffer + 1, data, size);
                return i2c_master_write_to_device(_cfg.i2c_port, _cfg.dev_addr, (const uint8_t*)buffer, size + 1, portMAX_DELAY);
            }

            inline esp_err_t _readReg(uint8_t reg, uint8_t* buffer, uint8_t readSize)
            {
                /* Store data into buffer */
                return i2c_master_write_read_device(_cfg.i2c_port, _cfg.dev_addr, &reg, 1, buffer, readSize, portMAX_DELAY);
            }


        public:

            /* Config */
            inline Config_t config() { return _cfg; }
            inline void config(const Config_t& cfg) { _cfg = cfg; }
            inline void setPin(const int& sda, const int& scl, const int& intr)
            {
                _cfg.pin_sda = sda;
                _cfg.pin_scl = scl;
                _cfg.pin_int = intr;
            }

            inline void cw_update_config_info() {
                _writeReg(REG_MODE, reg_buffer[MODE_NORMAL]);
                _writeReg(REG_BATINFO, (uint8_t*)cw_bat_config_info, sizeof(cw_bat_config_info));
                _writeReg(REG_CONFIG, reg_buffer[CONFIG_UPDATE_FLG] | reg_buffer[ATHD_DEFAULT]);
                _writeReg(REG_MODE, reg_buffer[MODE_RESTART]);
                _writeReg(REG_MODE, reg_buffer[MODE_NORMAL] | reg_buffer[MODE_QUICK_START]);
            }

            inline bool init(const int& sda, const int& scl, const int& intr = -1)
            {
                setPin(sda, scl, intr);
                return init();
            }

            inline bool init()
            {
                gpioInit();

                if(_writeReg(REG_MODE, reg_buffer[MODE_NORMAL]) != ESP_OK) {
                    ESP_LOGE(TAG, "i2c write failed");
                    return false;
                }
                cw_update_config_info();
                return true;
            }

            /* Setup gpio and reset */
            inline void gpioInit()
            {
                ESP_LOGD(TAG, "setup gpio");

                if (_cfg.pin_int > 0) {
                    gpio_reset_pin((gpio_num_t)_cfg.pin_int);
                    gpio_set_direction((gpio_num_t)_cfg.pin_int, GPIO_MODE_INPUT);
                }
            }

            /* Charging status */
            inline bool isCharging()
            {
                // TODO
                return false;
            }

            inline bool isChargeDone()
            {
                // TODO
                return false;
            }

            inline void update()
            {
                if(_readReg(REG_VCELL, _regs, sizeof(_regs)) != ESP_OK) {
                    ESP_LOGE(TAG, "i2c read failed");
                }
                voltage = ((_regs[0] << 8) | _regs[1]) * 305 / 1000;
                soc = _regs[2];
            }

            /* Bettery status */
            inline uint8_t batteryLevel()
            {
                /* Battery percentage data */
                update();
                uint16_t vol = batteryVoltage();
                for(int i = 0; i < 7; i++)
                    printf("%02x ", _regs[i]);
                printf("\nBatteryLevel: %i vol: %i\n", soc, vol);
                return soc;
            }

            inline uint16_t batteryVoltage()
            {
                return voltage;
            }


            /* Power control */
            inline void powerOff()
            {
            }

    };
}
