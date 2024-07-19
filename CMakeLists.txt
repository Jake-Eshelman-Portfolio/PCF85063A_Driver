cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(PCF85063A)

# Always include PCF85063A.c
target_sources(app PRIVATE src/PCF85063A.c)

# Conditional compilation based on UNIT_TEST
if(UNIT_TEST)
    
    target_sources(app PRIVATE
        tests/test_pcf85063a.c
    )
    
    target_include_directories(app PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${ZEPHYR_BASE}/subsys/testsuite/include
        ${ZEPHYR_UNITY_MODULE_DIR}/src
    )
    
    # Explicitly exclude the original main.c to avoid double main definition as main is defined in Ztest
    set_source_files_properties(
        ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c
        PROPERTIES
        EXCLUDE_FROM_ALL TRUE
    )
else()
    # Main application build
    target_sources(app PRIVATE src/main.c)
endif()

# Common Ztest configuration
if(CONFIG_ZTEST)
    target_include_directories(app PRIVATE 
        ${ZEPHYR_BASE}/subsys/testsuite/include
        ${ZEPHYR_UNITY_MODULE_DIR}/src
    )
endif()