idf_component_register(SRCS "hwifi.cpp" "soft_ap.cpp" "webserver.cpp"
        PRIV_REQUIRES log esp_wifi esp_netif mqtt esp_http_server espressif__fmt
        REQUIRES flash
        INCLUDE_DIRS "include")
target_compile_options(${COMPONENT_LIB} PRIVATE -std=gnu++20 -fno-exceptions -fno-rtti)