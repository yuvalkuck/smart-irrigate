idf_component_register(SRCS "1.5.0/bmp5.c"
        PRIV_REQUIRES common
        INCLUDE_DIRS "1.5.0")
#target_compile_options(${COMPONENT_LIB} PRIVATE -fno-exceptions -fno-rtti)
