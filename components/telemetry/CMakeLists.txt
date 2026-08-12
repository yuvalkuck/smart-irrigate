idf_component_register(SRCS "telemetry.cpp"
        "adc_sensors.cpp" "src/diagnostics.cpp" "src/sensor_sht4x.cpp" "src/sensor_bmp5xx.cpp"
        PRIV_REQUIRES i2cdev esp_sht4x esp_event esp_adc
        REQUIRES common bmp5api tsl2591 ds18b20
        PRIV_INCLUDE_DIRS inc
                    INCLUDE_DIRS "include")
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
target_compile_options(${COMPONENT_LIB} PRIVATE -std=gnu++20 -fno-exceptions -fno-rtti)