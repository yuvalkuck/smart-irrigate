idf_component_register(SRCS "hmodbus.cpp"
        REQUIRES common
        PRIV_REQUIRES log esp_driver_uart esp_driver_gpio espressif__esp-modbus esp_board_manager
        INCLUDE_DIRS "include")
target_compile_options(${COMPONENT_LIB} PRIVATE -std=gnu++20 -fno-exceptions -fno-rtti)
