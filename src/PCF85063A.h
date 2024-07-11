#ifndef PCF85063_H
#define PCF85063_H

#define PCF85063A_Address 0x51
#define PCF85063_register_size 18

#define SECONDS_INDEX 0
#define MINUTES_INDEX 1
#define HOURS_INDEX 2
#define DATE_INDEX 3
#define WEEKDAY_INDEX 4
#define MONTH_INDEX 5
#define YEAR_INDEX 6

#define RTC_TIME_REGISTER_ADDRESS 0x04
#define RTC_TIME_REGISTER_SIZE 7

#define DEVICE_SETUP_ERR -1
#define I2C_WRITE_ERR -2
#define I2C_READ_ERR -3
#define SUCCESS 0

uint8_t initialize_RTC(uint8_t * time_array);
uint8_t * get_civic_time();
uint8_t read_register(uint8_t * read_buffer, uint8_t size, uint8_t start_address);
uint8_t write_register(uint8_t * read_buffer, uint8_t size, uint8_t start_address);

#endif