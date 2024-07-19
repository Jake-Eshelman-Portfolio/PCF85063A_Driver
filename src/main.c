#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include "PCF85063A.h"

int main(void)
{
        uint8_t ret;
        uint8_t * time_array = get_civic_time();
        ret = initialize_RTC(time_array);
        return RTC_SUCCESS;
}