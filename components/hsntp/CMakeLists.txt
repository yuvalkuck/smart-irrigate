idf_component_register(SRCS "hsntp.cpp"
        PRIV_REQUIRES log esp_netif
        INCLUDE_DIRS "include")
target_compile_options(${COMPONENT_LIB} PRIVATE -std=gnu++20 -fno-exceptions -fno-rtti)