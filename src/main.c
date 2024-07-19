#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include "PCF85063A.h"

int main(void)
{
        uint8_t ret;
        uint8_t * time_array = get_civic_time();
        ret = initialize_RTC(time_array);
        if (ret != RTC_SUCCESS) {
                LOG_INF("RTC initialzation failed");
                return RTC_ERROR_DEVICE_SETUP;
        }
                 
        return RTC_SUCCESS;
}