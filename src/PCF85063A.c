#include <zephyr/drivers/i2c.h>
#include <string.h>
#include <stdlib.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include "PCF85063A.h"

const struct device *pcf_85063A;
static const struct gpio_dt_spec int_gpio = GPIO_DT_SPEC_GET(PCF85063A_INT_NODE, gpios);
static struct gpio_callback gpio_cb;

uint8_t convert_to_bcd(uint8_t decimal)
{
	return ((decimal / 10) << 4) | (decimal % 10);
}

uint8_t find_month(const char *month_str)
{
	const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
				"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
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
	char month[4] = {date_str[0], date_str[1], date_str[2], '\0'};
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
	char year[3] = {date_str[strlen(date_str) - 2], date_str[strlen(date_str) - 1], '\0'};
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
		printk("Error: i2c device is not ready\n");
		return DEVICE_SETUP_ERR;
	}

	if (!device_is_ready(int_gpio.port)) {
		printk("Error: interrupt GPIO device is not ready\n");
		return DEVICE_SETUP_ERR;
	}

	int ret = gpio_pin_configure_dt(&int_gpio, GPIO_INPUT);
	if (ret != 0) {
		printk("Error %d: failed to configure interrupt pin\n", ret);
		return DEVICE_SETUP_ERR;
	}

	ret = gpio_pin_interrupt_configure_dt(&int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		printk("Error %d: failed to configure interrupt\n", ret);
		return DEVICE_SETUP_ERR;
	}

	gpio_init_callback(&gpio_cb, alarm_callback, BIT(int_gpio.pin));
	gpio_add_callback(int_gpio.port, &gpio_cb);

	// Fill time array: sec, min, hr, day(1-31), weekday, month, year
	ret = i2c_burst_write(pcf_85063A, PCF85063A_Address, RTC_TIME_REGISTER_ADDRESS,
				  time_array, RTC_TIME_REGISTER_SIZE);

	if (ret != 0) {
		printk("Error %d: i2c burst write failed\n", ret);
		return I2C_WRITE_ERR;
	}

	return SUCCESS;
}

// Read from specified registers into provided read buffer, print result. Return read status code.
uint8_t read_register(uint8_t *read_buffer, uint8_t size, uint8_t start_address)
{
	uint8_t ret = 0;
	ret = i2c_burst_read(pcf_85063A, PCF85063A_Address, start_address, read_buffer, size);

	if (ret != SUCCESS) {
		return ret;
	}

	for (int i = 0; i < size; i++) {
		printk("Registers read: %02X \n", read_buffer[i]);
	}

	return SUCCESS;
}

// Write to specified registers from provided write buffer. Return read status code.
uint8_t write_register(uint8_t *write_buffer, uint8_t size, uint8_t start_address)
{
	uint8_t ret = 0;

	ret = i2c_burst_write(pcf_85063A, PCF85063A_Address, start_address, write_buffer, size);

	if (ret != SUCCESS) {
		return ret;
	}

	return SUCCESS;
}

void alarm_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    printk("Alarm triggered!\n");
}

uint8_t set_alarm()
{
	// Set the control register
	uint8_t control_2;
	control_2 |= (1 << 7);
	write_register(control_2, 1, 0x01);

	// Set the alarm register
	// sec, min, hour, day, weekday
	uint8_t alarm_buffer[RTC_ALARM_REGISTER_SIZE] = {0, 0x50, 0x17, 0x12, 0};
	write_register(alarm_buffer, RTC_TIME_REGISTER_SIZE, RTC_TIME_REGISTER_ADDRESS);


}