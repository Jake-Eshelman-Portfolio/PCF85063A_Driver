#include <zephyr/drivers/i2c.h>
#include <string.h>
#include <stdlib.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "PCF85063A.h"

LOG_MODULE_REGISTER(pcf85063a, CONFIG_PCF85063A_LOG_LEVEL);
static const struct device *pcf_85063A;
static const struct gpio_dt_spec int_gpio = GPIO_DT_SPEC_GET(PCF85063A_INT_NODE, gpios);
static struct gpio_callback gpio_cb;
static struct k_work alarm_work;
volatile bool alarm_trigger = false;


uint8_t extract_time_component(const char *time_str, int index)
{
    return ((time_str[index] - '0') << BCD_SHIFT) | (time_str[index + 1] - '0');
}

uint8_t convert_to_bcd(uint8_t decimal)
{
	return ((decimal / 10) << BCD_SHIFT) | (decimal % 10);
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

/**
 * @brief Returns the civic time in BCD format
 *
 * @return uint8_t* Pointer to an array containing:
 *         Sec, Min, Hr, day(1-31), Weekday, Month, Year
 * @note This is set at time of programming then maintained by RTC.
 *       If RTC power is lost make sure to rebuild and reprogram
 *       not just reprogram to maintain time.
 */
uint8_t *get_civic_time()
{
    const char *time_str = __TIME__;
    const char *date_str = __DATE__;
    static uint8_t time_array[7];

    // Extract month
    char month[DATE_MONTH_END + 1];
    strncpy(month, date_str, DATE_MONTH_END);
    month[DATE_MONTH_END] = '\0';
    uint8_t decimal_month = find_month(month);
    uint8_t bcd_month = convert_to_bcd(decimal_month);

    // Extract date
    char date[3];
    int j = DATE_DAY_START, k = 0;
    while (date_str[j] != ' ' && k < sizeof(date) - 1) {
        date[k++] = date_str[j++];
    }
    date[k] = '\0';
    uint8_t decimal_date = atoi(date);
    uint8_t bcd_date = convert_to_bcd(decimal_date);

    // Extract year
    char year[YEAR_DIGITS + 1];
    strncpy(year, &date_str[strlen(date_str) - YEAR_DIGITS], YEAR_DIGITS);
    year[YEAR_DIGITS] = '\0';
    uint8_t decimal_year = atoi(year);
    uint8_t bcd_year = convert_to_bcd(decimal_year);

    // Extract time
    uint8_t hours = extract_time_component(time_str, TIME_HOURS_INDEX);
    uint8_t minutes = extract_time_component(time_str, TIME_MINUTES_INDEX);
    uint8_t seconds = extract_time_component(time_str, TIME_SECONDS_INDEX);

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

/**
 * @brief Callback function for alarm interrupt
 *
 * @param dev Pointer to the device structure
 * @param cb Pointer to the GPIO callback structure
 * @param pins The pin mask for the callback
 */
static void alarm_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	LOG_INF("Alarm triggered! \n");
	k_work_submit(&alarm_work);
}

/**
 * @brief Set the time for the RTC
 *
 * @param time_array Pointer to an array containing time in format:
 *                   sec, min, hr, day(1-31), weekday, month, year
 * @return rtc_error_t Status of initialization and write
 */
rtc_error_t initialize_RTC(const uint8_t *time_array)
{
	if (time_array == NULL) {
		LOG_ERR("Time array was null \n");
		return RTC_ERROR_INVALID_PARAMETER;
	}

	// Validate alarm time values
    if (time_array[SECONDS_INDEX] > 0x59 ||
        time_array[MINUTES_INDEX] > 0x59 ||
        time_array[HOURS_INDEX] > 0x23 ||
        time_array[DATE_INDEX] > 0x31 ||
        time_array[WEEKDAY_INDEX] != 0 ||
		time_array[MONTH_INDEX] > 0x31 ||
        time_array[YEAR_INDEX] == 0) {

        LOG_ERR("Invalid time array values \n");
        return RTC_ERROR_INVALID_PARAMETER;
    }


	pcf_85063A = DEVICE_DT_GET(DT_NODELABEL(i2c0));
	if (!device_is_ready(pcf_85063A)) {
		LOG_ERR("Error: i2c device is not ready\n");
		return RTC_ERROR_DEVICE_SETUP;
	}

	if (!device_is_ready(int_gpio.port)) {
		LOG_ERR("Error: interrupt GPIO device is not ready\n");
		return RTC_ERROR_DEVICE_SETUP;
	}

	int ret = gpio_pin_configure_dt(&int_gpio, GPIO_INPUT);
	if (ret != RTC_SUCCESS) {
		LOG_ERR("Error %d: failed to configure interrupt pin\n", ret);
		return RTC_ERROR_DEVICE_SETUP;
	}

	ret = gpio_pin_interrupt_configure_dt(&int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != RTC_SUCCESS) {
		LOG_ERR("Error %d: failed to configure interrupt\n", ret);
		return RTC_ERROR_DEVICE_SETUP;
	}

	gpio_init_callback(&gpio_cb, alarm_callback, BIT(int_gpio.pin));
	gpio_add_callback(int_gpio.port, &gpio_cb);

	// Write the time array to the registers
	ret = i2c_burst_write(pcf_85063A, RTC_ADDRESS, RTC_TIME_REGISTER_ADDRESS, time_array,
			      RTC_TIME_REGISTER_SIZE);

	if (ret != RTC_SUCCESS) {
		LOG_ERR("Error %d: i2c burst write failed\n", ret);
		return RTC_ERROR_I2C_WRITE;
	}

	return RTC_SUCCESS;
}

/**
 * @brief Read from specified registers into provided read buffer
 *
 * @param read_buffer Pointer to the buffer to store read data
 * @param size Number of bytes to read
 * @param start_address Starting address to read from
 * @return rtc_error_t Read status code
 * @note Prints the result
 */
rtc_error_t read_register(uint8_t *read_buffer, const uint8_t size, const uint8_t start_address)
{
	// Input validation. Buffer is circular so reads beyond 18 are valid, but should not be allowed for clarity.
	if (read_buffer == NULL) {
		LOG_ERR("read_buffer was null \n");
		return RTC_ERROR_INVALID_PARAMETER;
	}

	if (size <= 0 || size > RTC_REGISTER_SIZE) {
		LOG_ERR("Invalid size: %d, size must be between 0 and 18 \n", size);
		return RTC_ERROR_INVALID_PARAMETER;
	}

	if (start_address < 0 || start_address > RTC_REGISTER_SIZE ) {
		LOG_ERR("Invalid start address: %d, start address must be between 0 and 17 \n", start_address);
		return RTC_ERROR_INVALID_PARAMETER;
	}

	if((start_address + size) > RTC_REGISTER_SIZE) {
		LOG_ERR("Invalid read range, outranged RTC registers");
		return RTC_ERROR_INVALID_PARAMETER;
	}

	uint8_t ret = 0;
	ret = i2c_burst_read(pcf_85063A, RTC_ADDRESS, start_address, read_buffer, size);

	if (ret != RTC_SUCCESS) {
		LOG_ERR("Error %d: burst read failed \n", ret);
		return RTC_ERROR_I2C_WRITE;
	}

	for (int i = 0; i < size; i++) {
		LOG_INF("Register[%d]: %02X \n", i, read_buffer[i]);
	}

	return RTC_SUCCESS;
}

/**
 * @brief Write to specified registers from provided write buffer
 *
 * @param write_buffer Pointer to the buffer containing data to write
 * @param size Number of bytes to write
 * @param start_address Starting address to write to
 * @return rtc_error_t Write status code
 */
rtc_error_t write_register(const uint8_t *write_buffer, const uint8_t size, const uint8_t start_address)
{
	// Input validation. Buffer is circular so writes beyond 18 are valid, but should not be allowed for clarity.
	if (write_buffer == NULL) {
		LOG_ERR("Write buffer was null \n");
		return RTC_ERROR_INVALID_PARAMETER;
	}

	if (size <= 0 || size > RTC_REGISTER_SIZE) {
		LOG_ERR("Invalid size: %d, size must be between 0 and 18 \n", size);
		return RTC_ERROR_INVALID_PARAMETER;
	}

	if (start_address < 0 || start_address > RTC_REGISTER_SIZE ) {
		LOG_ERR("Invalid start address: %d, start address must be between 0 and 17 \n", start_address);
		return RTC_ERROR_INVALID_PARAMETER;
	}

	if((start_address + size) > RTC_REGISTER_SIZE) {
		LOG_ERR("Invalid write range, outranged RTC registers \n");
		return RTC_ERROR_INVALID_PARAMETER;
	}

	uint8_t ret = 0;
	ret = i2c_burst_write(pcf_85063A, RTC_ADDRESS, start_address, write_buffer, size);

	if (ret != RTC_SUCCESS) {
		LOG_ERR("Error %d: burst write failed \n", ret);
		return RTC_ERROR_I2C_WRITE;
	}

	return RTC_SUCCESS;
}

/**
 * @brief Placeholder function, currently reads all registers when alarm is called
 *
 * @param work Pointer to the work structure
 */
static void alarm_work_handler(struct k_work *work)
{
	uint8_t read_buffer[RTC_REGISTER_SIZE] = {0};
	read_register(read_buffer, RTC_REGISTER_SIZE, 0X00);
	alarm_trigger = true;
}


/**
 * @brief Set the alarm with a given time in BCD format
 *
 * @param alarm_buffer Pointer to an array containing:
 *                     Sec, min, hour, day, weekday
 * @param size Size of the alarm_buffer
 * @return rtc_error_t Status of the alarm setting operation
 * @note Triggers interrupt work function when time is reached
 */
rtc_error_t set_alarm(const uint8_t *alarm_buffer, const size_t size)
{
	// Input validation
    if (alarm_buffer == NULL) {
        LOG_ERR("Alarm buffer is NULL \n");
        return RTC_ERROR_INVALID_PARAMETER;
    }

    if (size != RTC_ALARM_REGISTER_SIZE) {
        LOG_ERR("Invalid alarm buffer size. Expected %d, got %d \n", 
                RTC_ALARM_REGISTER_SIZE, size);
        return RTC_ERROR_INVALID_PARAMETER;
    }

    // Validate alarm time values
    if (alarm_buffer[SECONDS_INDEX] > 0x59 ||
        alarm_buffer[MINUTES_INDEX] > 0x59 ||
        alarm_buffer[HOURS_INDEX] > 0x23 ||
        alarm_buffer[DATE_INDEX] > 0x31 ||
        alarm_buffer[WEEKDAY_INDEX] != 0) {
        LOG_ERR("Invalid alarm time values \n");
        return RTC_ERROR_INVALID_PARAMETER;
    }

	// Set the control register
	uint8_t control_2[ALARM_CONTROL_REGISTER], ret = 0;
	control_2[0] = ENABLE_ALARM;
	ret = write_register(control_2, sizeof(control_2), ALARM_CONTROL_REGISTER);
	if (ret != RTC_SUCCESS) {
		LOG_ERR("Error: %d. Failed to set alarm control register due to burst write failure \n", ret);
		return RTC_ERROR_I2C_WRITE;
	}

	// Set the alarm register
	ret = write_register(alarm_buffer, size, RTC_ALARM_REGISTER_ADDRESS);
	if (ret != RTC_SUCCESS) {
		LOG_ERR("Error: %d. Failed to set alarm time register due to burst write failure \n", ret);
		return RTC_ERROR_I2C_WRITE;
	}
	k_work_init(&alarm_work, alarm_work_handler);
	return RTC_SUCCESS;
}
