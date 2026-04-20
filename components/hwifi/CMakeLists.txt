idf_component_register(SRCS "hwifi.cpp"
        PRIV_REQUIRES log esp_wifi esp_netif mqtt
                    INCLUDE_DIRS "include")
target_compile_options(${COMPONENT_LIB} PRIVATE -std=gnu++20 -fno-exceptions -fno-rtti)