idf_component_register(SRCS "hmqtt.cpp"
        REQUIRES flash common
        PRIV_REQUIRES log mqtt
        EMBED_TXTFILES "../../certs/ca.crt" "../../certs/client.crt" "../../certs/client.key"
        INCLUDE_DIRS "include")
target_compile_options(${COMPONENT_LIB} PRIVATE -std=gnu++20 -fno-exceptions -fno-rtti)
