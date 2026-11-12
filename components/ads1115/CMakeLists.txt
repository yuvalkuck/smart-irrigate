idf_component_register(SRCS "ads1115.cpp"
        REQUIRES esp_driver_i2c
        PRIV_REQUIRES log freertos
        INCLUDE_DIRS "include")
target_compile_options(${COMPONENT_LIB} PRIVATE -std=gnu++20 -fno-exceptions -fno-rtti)
