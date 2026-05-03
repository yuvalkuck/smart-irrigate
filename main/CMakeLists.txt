idf_component_register(SRCS "entrypoint.cpp"
        REQUIRES flash hwifi hmqtt hsntp blesrv
        PRIV_REQUIRES log esp_event esp_driver_gpio mqtt
        INCLUDE_DIRS "."
)
target_compile_options(${COMPONENT_LIB} PRIVATE -std=gnu++20 -fno-exceptions -fno-rtti)
