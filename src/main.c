#include <kernel.h>
#include <sys/printk.h>
#include "PCF85063A.h"

int main(void)
{
        uint8_t * time_array = get_civic_time();
        uint8_t read_buffer[RTC_TIME_REGISTER_SIZE];
        uint8_t write_buffer[RTC_TIME_REGISTER_SIZE] = {0,0,0,0,0,0,0};
        initialize_RTC(time_array);
        read_register(read_buffer, RTC_TIME_REGISTER_SIZE, RTC_TIME_REGISTER_ADDRESS);
        write_register(write_buffer, 7, 4);
        read_register(read_buffer, RTC_TIME_REGISTER_SIZE, RTC_TIME_REGISTER_ADDRESS);
        return 0;
}
