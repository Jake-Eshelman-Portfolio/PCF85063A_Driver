#ifndef PCF85063_H
#define PCF85063_H

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

#define SECONDS_INDEX 0
#define MINUTES_INDEX 1
#define HOURS_INDEX 2
#define DATE_INDEX 3
#define WEEKDAY_INDEX 4
#define MONTH_INDEX 5
#define YEAR_INDEX 6
#define TIME_HOURS_INDEX 0
#define TIME_MINUTES_INDEX 3
#define TIME_SECONDS_INDEX 6
#define DATE_MONTH_END 3
#define DATE_DAY_START 4
#define YEAR_DIGITS 2
#define BCD_SHIFT 4
#define CONFIG_PCF85063A_LOG_LEVEL 3

#define RTC_ADDRESS 0x51
#define RTC_REGISTER_SIZE 18
#define RTC_ALARM_REGISTER_SIZE 5
#define RTC_TIME_REGISTER_ADDRESS 0x04
#define RTC_ALARM_REGISTER_ADDRESS 0x0B
#define RTC_TIME_REGISTER_SIZE 7
#define ENABLE_ALARM 0x80
#define ALARM_CONTROL_REGISTER 1

#define PCF85063A_INT_NODE DT_NODELABEL(pcf85063a_int1)

typedef enum {
    RTC_SUCCESS = 0,
    RTC_ERROR_DEVICE_SETUP = -1,
    RTC_ERROR_I2C_WRITE = -2,
    RTC_ERROR_I2C_READ = -3,
    RTC_ERROR_INVALID_PARAMETER = -4,
    RTC_ERROR_GPIO_CONFIG = -5,
} rtc_error_t;


uint8_t * get_civic_time();

rtc_error_t initialize_RTC(const uint8_t *time_array);
rtc_error_t read_register(uint8_t *read_buffer, const uint8_t size, const uint8_t start_address);
rtc_error_t write_register(const uint8_t *write_buffer, const uint8_t size, const uint8_t start_address);

rtc_error_t set_alarm(const uint8_t *alarm_buffer, const size_t size);



#endif