#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include "PCF85063A.h"

int main(void)
{
        printk("hello");
        uint8_t * time_array = get_civic_time();
        uint8_t * read_buffer;
        initialize_RTC(time_array);
        read_register(read_buffer, 7, 4);
        return 0;
}
