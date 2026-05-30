idf_component_register(SRCS "telemetry.cpp"
        PRIV_REQUIRES bme680 i2cdev
                    INCLUDE_DIRS "include")
target_compile_options(${COMPONENT_LIB} PRIVATE -std=gnu++20 -fno-exceptions -fno-rtti)