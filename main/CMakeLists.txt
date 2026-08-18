idf_component_register(SRCS "entrypoint.cpp" "event_handler.cpp"
        REQUIRES flash hwifi hmqtt hsntp telemetry common protocol
        PRIV_REQUIRES log esp_event esp_driver_gpio mqtt esp_adc
        INCLUDE_DIRS "."
)
# esp_driver_uart
target_compile_options(${COMPONENT_LIB} PRIVATE -std=gnu++20 -fno-exceptions -fno-rtti)
