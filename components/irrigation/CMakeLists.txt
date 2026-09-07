idf_component_register(SRCS "irrigation.cpp"
        REQUIRES protocol common
        PRIV_REQUIRES esp_driver_gpio esp_timer log
        INCLUDE_DIRS "include"
)
target_compile_options(${COMPONENT_LIB} PRIVATE -std=gnu++20 -fno-exceptions -fno-rtti)
