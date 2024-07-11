#include <kernel.h>
#include <sys/printk.h>
#include "PCF85063A.h"

int main(void)
{
        uint8_t * time_array = get_civic_time();
        uint8_t read_buffer[RTC_TIME_REGISTER_SIZE];
        initialize_RTC(time_array);
        read_register(read_buffer, RTC_TIME_REGISTER_SIZE, RTC_TIME_REGISTER_ADDRESS);
        return 0;
}
