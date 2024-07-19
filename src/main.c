#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "PCF85063A.h"

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

int main(void)
{
        uint8_t ret;
        uint8_t * time_array = get_civic_time();
        ret = initialize_RTC(time_array);
        if (ret != RTC_SUCCESS) {
                LOG_INF("RTC initialzation failed");
                return RTC_ERROR_DEVICE_SETUP;
        }
        else {
                LOG_INF("RTC initialized successfully");
        }
                 
        return RTC_SUCCESS;
}