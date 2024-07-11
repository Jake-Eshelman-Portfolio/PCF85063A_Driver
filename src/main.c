#include <kernel.h>
#include <sys/printk.h>
#include "PCF85063A.h"

int main(void)
{
        uint8_t * time_array = get_civic_time();
        initialize_RTC(time_array);
        return 0;
}
