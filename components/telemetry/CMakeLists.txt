idf_component_register(SRCS "telemetry.cpp" "diagnostics.cpp" "adc_sensors.cpp"
        PRIV_REQUIRES i2cdev esp_sht4x esp_event esp_adc
        REQUIRES common
                    INCLUDE_DIRS "include")
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
target_compile_options(${COMPONENT_LIB} PRIVATE -std=gnu++20 -fno-exceptions -fno-rtti)