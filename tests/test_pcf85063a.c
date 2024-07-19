#include <zephyr/ztest.h>
#include "PCF85063A.h"

//#define TEST_ALARM


ZTEST_SUITE(pcf85063a_tests, NULL, NULL, NULL, NULL, NULL);

ZTEST(pcf85063a_tests, test_convert_to_bcd)
{
    zassert_equal(convert_to_bcd(0), 0x00, "convert_to_bcd failed for 0");
    zassert_equal(convert_to_bcd(12), 0x12, "convert_to_bcd failed for 12");
    zassert_equal(convert_to_bcd(59), 0x59, "convert_to_bcd failed for 59");
    zassert_equal(convert_to_bcd(99), 0x99, "convert_to_bcd failed for 99");
}

ZTEST(pcf85063a_tests, test_find_month)
{
    zassert_equal(find_month("Jan"), 1, "find_month failed for Jan");
    zassert_equal(find_month("Dec"), 12, "find_month failed for Dec");
    zassert_equal(find_month("Invalid"), 0, "find_month failed for invalid month");
}

ZTEST(pcf85063a_tests, test_extract_time_component)
{
    const char *time_str = "12:34:56";
    zassert_equal(extract_time_component(time_str, 0), 0x12, "extract_time_component failed for hours");
    zassert_equal(extract_time_component(time_str, 3), 0x34, "extract_time_component failed for minutes");
    zassert_equal(extract_time_component(time_str, 6), 0x56, "extract_time_component failed for seconds");
}

ZTEST(pcf85063a_tests, test_get_civic_time)
{
    uint8_t *time_array = get_civic_time();
    zassert_not_null(time_array, "get_civic_time returned NULL");
    
    // Verify the time format (e.g., hours, minutes, seconds)
    // Assuming time_array is in [seconds, minutes, hours, day, date, month, year] format
    zassert_true(time_array[0] <= 0x60, "Invalid seconds value %d", time_array[0]);
    zassert_true(time_array[1] <= 0x60, "Invalid minutes value");
    zassert_true(time_array[2] < 0x24, "Invalid hours value");
    zassert_true(time_array[3] > 0x00 && time_array[3] <= 0x31, "Invalid day value, %d", time_array[3]);
    zassert_true(time_array[4] == 0, "Invalid date value"); // This is not set as it is unncessary in clock and difficult to obtain
    zassert_true(time_array[5] >= 0x01 && time_array[5] <= 0x012, "Invalid month value");
    zassert_true(time_array[6] <= 0x99, "Invalid year value");

    // Verify consistency
    uint8_t *time_array2 = get_civic_time();
    zassert_not_null(time_array2, "get_civic_time returned NULL on second call");
    zassert_mem_equal(time_array, time_array2, RTC_TIME_REGISTER_SIZE, "Inconsistent time returned by get_civic_time");
    
    // Check the time format and range in more detail
    // Assuming the function returns an array of uint8_t in BCD format
    for (int i = 0; i < RTC_TIME_REGISTER_SIZE; i++) {
        zassert_true((time_array[i] & 0xF0) <= 0x90, "Invalid BCD format (high nibble) at index %d", i);
        zassert_true((time_array[i] & 0x0F) <= 0x09, "Invalid BCD format (low nibble) at index %d", i);
    }
    
    // Optionally simulate edge cases (e.g., near midnight)
}

ZTEST(pcf85063a_tests, test_initialize_rtc)
{
    uint8_t * time_array = get_civic_time();
    rtc_error_t ret = initialize_RTC(time_array);
    zassert_equal(ret, RTC_SUCCESS, "initialize_RTC failed");
    
    // Verify the time was set correctly
    uint8_t read_buffer[RTC_TIME_REGISTER_SIZE];
    ret = read_register(read_buffer, RTC_TIME_REGISTER_SIZE, RTC_TIME_REGISTER_ADDRESS);
    zassert_equal(ret, RTC_SUCCESS, "Failed to read back time");
    zassert_mem_equal(time_array, read_buffer, RTC_TIME_REGISTER_SIZE, "Time not set correctly");

    // Verify clock is keeping time correctly
    // Set current time (e.g., 12:00:00)
    uint8_t current_time[RTC_TIME_REGISTER_SIZE] = {0x00, 0x00, 0x12, 0x15, 0x00, 0x07, 0x23};
    ret = write_register(current_time, sizeof(current_time), RTC_TIME_REGISTER_ADDRESS);
    zassert_equal(ret, RTC_SUCCESS, "Failed to set current time");

    k_msleep(3000);
    ret = read_register(read_buffer, RTC_TIME_REGISTER_SIZE, RTC_TIME_REGISTER_ADDRESS);
    zassert_equal(ret, RTC_SUCCESS, "Failed to read back time");
    zassert_true(read_buffer[0] > 2, "Clock seconds not progressing correctly");
}

ZTEST(pcf85063a_tests, test_read_write_register)
{
    uint8_t write_buffer[3] = {0x34, 0x56, 0x78};
    rtc_error_t ret = write_register(write_buffer, sizeof(write_buffer), 0x01);
    zassert_equal(ret, RTC_SUCCESS, "write_register failed");

    uint8_t read_buffer[3];
    ret = read_register(read_buffer, sizeof(read_buffer), 0x01);
    zassert_equal(ret, RTC_SUCCESS, "read_register failed");
    zassert_mem_equal(write_buffer, read_buffer, sizeof(read_buffer), "Read data doesn't match written data");
}

ZTEST(pcf85063a_tests, test_set_alarm)
{
    uint8_t alarm_buffer[RTC_ALARM_REGISTER_SIZE] = {0x30, 0x45, 0x12, 0x15, 0x00};
    rtc_error_t ret = set_alarm(alarm_buffer, RTC_ALARM_REGISTER_SIZE);
    zassert_equal(ret, RTC_SUCCESS, "set_alarm failed");
    
    // Verify the alarm was set correctly
    uint8_t read_buffer[RTC_ALARM_REGISTER_SIZE];
    ret = read_register(read_buffer, RTC_ALARM_REGISTER_SIZE, RTC_ALARM_REGISTER_ADDRESS);
    zassert_equal(ret, RTC_SUCCESS, "Failed to read back alarm");
    zassert_mem_equal(alarm_buffer, read_buffer, RTC_ALARM_REGISTER_SIZE, "Alarm not set correctly");
}

ZTEST(pcf85063a_tests, test_alarm)
{
    // Set current time (e.g., 12:00:00)
    uint8_t current_time[RTC_TIME_REGISTER_SIZE] = {0x00, 0x00, 0x12, 0x15, 0x00, 0x07, 0x23};
    rtc_error_t ret = initialize_RTC(current_time);
    zassert_equal(ret, RTC_SUCCESS, "Failed to set current time");

    // Set alarm for 5 seconds later (12:00:05)
    uint8_t alarm_time[RTC_ALARM_REGISTER_SIZE] = {0x02, 0x00, 0x12, 0x15, 0x00};
    ret = set_alarm(alarm_time, RTC_ALARM_REGISTER_SIZE);
    zassert_equal(ret, RTC_SUCCESS, "Failed to set alarm");

    // Wait for alarm to trigger (with timeout)
    int64_t start_time = k_uptime_get();
    while (!alarm_trigger && (k_uptime_get() - start_time < 10000)) {
        k_sleep(K_MSEC(100));
    }

    // Check if alarm triggered
    zassert_true(alarm_trigger, "Alarm did not trigger within expected time");
}

ZTEST(pcf85063a_tests, test_error_handling)
{
    zassert_equal(initialize_RTC(NULL), RTC_ERROR_INVALID_PARAMETER, "initialize_RTC should fail with NULL input");
    zassert_equal(read_register(NULL, 1, 0), RTC_ERROR_INVALID_PARAMETER, "read_register should fail with NULL buffer");
    zassert_equal(write_register(NULL, 1, 0), RTC_ERROR_INVALID_PARAMETER, "write_register should fail with NULL buffer");
    zassert_equal(set_alarm(NULL, RTC_ALARM_REGISTER_SIZE), RTC_ERROR_INVALID_PARAMETER, "set_alarm should fail with NULL buffer");
}