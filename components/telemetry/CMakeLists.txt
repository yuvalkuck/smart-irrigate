idf_component_register(SRCS "telemetry.cpp"
        "src/sensor_sht4x.cpp" "src/sensor_bmp5xx.cpp" "src/sensor_ds18b20.cpp" "src/sensor_xdb401.cpp" "src/sensor_wind.cpp"
            PRIV_REQUIRES i2cdev esp_sht4x esp_event esp_adc
            REQUIRES common bmp5api ds18b20 onewire_bus
            PRIV_INCLUDE_DIRS inc
            INCLUDE_DIRS "include"
)
SET(CMAKE_C_STANDARD 99)
SET(CMAKE_C_STANDARD_REQUIRED ON)
target_compile_options(${COMPONENT_LIB} PRIVATE -std=gnu++20 -fno-exceptions -fno-rtti)

# "src/sensor_tsl2591.cpp" # tsl2591
