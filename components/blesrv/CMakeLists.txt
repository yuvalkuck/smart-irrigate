IDF_COMPONENT_REGISTER(SRCS "blesrv.cpp" "debug_event_name.c" "services.cpp"
        PRIV_REQUIRES log bt nvs_flash
        INCLUDE_DIRS "include")
TARGET_COMPILE_OPTIONS(${COMPONENT_LIB} PRIVATE -std=gnu++20 -fno-exceptions -fno-rtti)
IF ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    TARGET_COMPILE_DEFINITIONS(${COMPONENT_LIB} PRIVATE CONFIG_BLE_DEBUG_EVENTS)
ENDIF ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
