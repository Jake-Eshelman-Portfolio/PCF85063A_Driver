```markdown
# PCF85063A RTC Driver Overview

## Introduction
This driver provides an interface for the PCF85063A Real-Time Clock (RTC) chip, designed for use with the Zephyr RTOS, and was built on the nRF52dk with testing through Unity/Ztest. It enables precise timekeeping and alarm functionality in embedded systems.

## Key Features
- RTC initialization and configuration
- Time setting and reading
- Alarm configuration and handling
- Register-level read and write operations
- Unit testing through Unity/Ztest

## Architecture
The driver is structured around core RTC operations, utilizing Zephyr's I2C and GPIO subsystems with asynchronous interrupt handling. It employs a modular design with separate functions for each major operation, enhancing maintainability and allowing for easy expansion of functionality. 

## Integration
To integrate this driver into your Zephyr project:
1. Include the driver files (PCF85063A.c and PCF85063A.h) in your project.
2. Configure your project's devicetree to include the PCF85063A device.
3. Enable necessary Zephyr configurations (I2C, GPIO) in your project's .conf file.

## Builds and Testing
- build is the default build, enables all functionality of RTC
- build_unity conditionally compiles Ztest and the test directory. To run tests:
    1. build_unity
    2. Erase and flash to board
    3. pcf85063a_tests will run and print results to serial output

## Usage
Basic usage examples:

```c
// Initializing the RTC
uint8_t * time_array = get_civic_time();
rtc_error_t ret = initialize_RTC(time_array);

// Setting an alarm
uint8_t alarm_buffer[RTC_ALARM_REGISTER_SIZE] = {0x10, 0x10, 0x10, 0x12, 0};
ret = set_alarm(alarm_buffer, RTC_ALARM_REGISTER_SIZE);
```

For detailed API usage, refer to the Doxygen-generated documentation: /docs/html/index.html

## Dependencies
- Zephyr RTOS (tested with version 1.0)
- Zephyr I2C Driver
- Zephyr GPIO Driver

## Known Limitations
- Currently supports only 24-hour time format
- Alarm functionality limited to date/time matching (no periodic alarms)

## Future improvements:
- Add support for periodic alarms
- Implement power management features of the PCF85063A
- Implement timer features
- Extend test suite to cover a wider range of cases for read/write operations
- Add python based interface for ease of programming and setting alarm



## Additional resources
- [PCF85063A Datasheet](https://www.nxp.com/docs/en/data-sheet/PCF85063A.pdf)
- [Zephyr RTOS Documentation](https://docs.zephyrproject.org/)
```