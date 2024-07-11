#include <drivers/i2c.h>
#include <string.h>
#include <stdlib.h>
#include "PCF85063A.h"

const struct device *pcf_85063A;

uint8_t convert_to_bcd(uint8_t decimal)
{
	return ((decimal / 10) << 4) | (decimal % 10);
}

uint8_t find_month(const char *month_str)
{
	const char *months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
				 "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
	for (int i = 0; i < 12; i++) {
		if (strcmp(month_str, months[i]) == 0) {
			return i + 1;
		}
	}
	return 0;
}

uint8_t *get_civic_time()
{
	const char *time_str = __TIME__;
	const char *date_str = __DATE__;
	static uint8_t time_array[7];

	// Extract month
	char month[4] = { date_str[0], date_str[1], date_str[2], '\0' };
	uint8_t decimal_month = find_month(month);
	uint8_t bcd_month = convert_to_bcd(decimal_month);

	// Extract date
	char date[3];
	uint8_t j = 4, k = 0;
	while (date_str[j] != ' ') {
		date[k++] = date_str[j++];
	}
	date[k] = '\0';
	uint8_t decimal_date = atoi(date);
	uint8_t bcd_date = convert_to_bcd(decimal_date);

	// Extract year
	char year[3] = { date_str[strlen(date_str) - 2], date_str[strlen(date_str) - 1], '\0' };
	uint8_t decimal_year = atoi(year);
	uint8_t bcd_year = convert_to_bcd(decimal_year);

	// Extract time
	uint8_t hours = ((time_str[0] - '0') << 4) | (time_str[1] - '0');
	uint8_t minutes = ((time_str[3] - '0') << 4) | (time_str[4] - '0');
	uint8_t seconds = ((time_str[6] - '0') << 4) | (time_str[7] - '0');

	// Fill time array: sec, min, hr, day(1-31), weekday, month, year
	time_array[SECONDS_INDEX] = seconds;
	time_array[MINUTES_INDEX] = minutes;
	time_array[HOURS_INDEX] = hours;
	time_array[DATE_INDEX] = bcd_date;
	time_array[WEEKDAY_INDEX] = 0; // Weekday is not set
	time_array[MONTH_INDEX] = bcd_month;
	time_array[YEAR_INDEX] = bcd_year;

	return time_array;
}

// Set the time for the RTC, returns status of initialization and write
uint8_t initialize_RTC(uint8_t *time_array)
{
	pcf_85063A = DEVICE_DT_GET(DT_NODELABEL(i2c0));
	if (!device_is_ready(pcf_85063A)) {
		return DEVICE_SETUP_ERR;
	} 

	// Fill time array: sec, min, hr, day(1-31), weekday, month, year
	int ret = i2c_burst_write(pcf_85063A, PCF85063A_Address, RTC_TIME_REGISTER_ADDRESS, time_array, RTC_TIME_REGISTER_SIZE);

	if (ret != 0) {
		return I2C_WRITE_ERR;
	}

	if (ret != 0) {
		return I2C_READ_ERR;
	}

	return SUCCESS;
}

void read_register(uint8_t * read_buffer, uint8_t size, uint8_t start_address)
{
	uint8_t ret = 0;
	ret = i2c_burst_read(pcf_85063A, PCF85063A_Address, start_address, read_buffer, size);

	for(int i = 0; i < size; i++)
	{
		printk("Registers read: %02X \n", read_buffer[i]);
	}

}
