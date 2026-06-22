idf_component_register(SRCS "blesrv.cpp" "debug_event_name.c" "services.cpp"
        PRIV_REQUIRES log bt nvs_flash
                    INCLUDE_DIRS "include")
target_compile_options(${COMPONENT_LIB} PRIVATE -std=gnu++20 -fno-exceptions -fno-rtti)
IF (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    TARGET_COMPILE_DEFINITIONS(${COMPONENT_LIB} PRIVATE CONFIG_BLE_DEBUG_EVENTS)
ENDIF (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
