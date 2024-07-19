#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include "PCF85063A.h"

int main(void)
{
        uint8_t ret;
        // this programs the build time and doesnt update on flashing
        uint8_t * time_array = get_civic_time();
        uint8_t read_buffer[18];

        // Fill time array: sec, min, hr, day(1-31), weekday, month, year
        uint8_t write_buffer[RTC_TIME_REGISTER_SIZE] = {0,0x10,0x10,0x12,0,0x7,0x24};
        // ret = initialize_RTC(time_array);
        ret = initialize_RTC(time_array);
        uint8_t alarm_buffer[RTC_ALARM_REGISTER_SIZE] = {0x10, 0x10, 0x10, 0x12, 0};
        read_register(read_buffer,18, 0X00);
        set_alarm(alarm_buffer, 5);


        //printk("init ret: %d \n", ret);
        return 0;
}