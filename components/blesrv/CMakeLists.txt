idf_component_register(SRCS "blesrv.cpp"
        PRIV_REQUIRES log
                    INCLUDE_DIRS "include")
target_compile_options(${COMPONENT_LIB} PRIVATE -std=gnu++20 -fno-exceptions -fno-rtti)