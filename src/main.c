#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include "PCF85063A.h"

int main(void)
{
        uint8_t ret;
        uint8_t * time_array = get_civic_time();
        uint8_t read_buffer[RTC_TIME_REGISTER_SIZE];
        uint8_t write_buffer[RTC_TIME_REGISTER_SIZE] = {0,0,0,0,0,0,0};
        ret = initialize_RTC(time_array);
        read_register(read_buffer,RTC_TIME_REGISTER_SIZE, 0X04);
        printk("init ret: %d \n", ret);
        return 0;
}