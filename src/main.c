#include <kernel.h>
#include <sys/printk.h>
#include "PCF85063A.h"

int main(void)
{
        uint8_t * time_array = get_civic_time();
        uint8_t read_buffer[RTC_TIME_REGISTER_SIZE];
        uint8_t write_buffer[RTC_TIME_REGISTER_SIZE] = {0,0,0,0,0,0,0};
        initialize_RTC(time_array);
        return 0;
}
