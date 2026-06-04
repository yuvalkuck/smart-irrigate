idf_component_register(SRCS "flash.cpp"
        PRIV_REQUIRES nvs_flash log common
        INCLUDE_DIRS "include")
target_compile_options(${COMPONENT_LIB} PRIVATE -std=gnu++20 -fno-exceptions -fno-rtti)
