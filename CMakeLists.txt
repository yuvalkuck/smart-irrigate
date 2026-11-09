# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)
set(IDF_TARGET esp32c6 CACHE STRING "cpuType" FORCE)
IF ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    ADD_COMPILE_DEFINITIONS(_DEBUG)
ENDIF ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(smart-irrigate
        VERSION 1.0.0.0
        LANGUAGES C CXX
)
SET(INITIATE_WIFI_SSID "$ENV{INITIATE_WIFI_SSID}")
SET(INITIATE_WIFI_PASSWORD "$ENV{INITIATE_WIFI_PASSWORD}")
# BT
SET(INITIATE_DEVICE_UNIQUE "$ENV{INITIATE_DEVICE_UNIQUE}")
#
# Mqtt STA Configuration
#
SET(INITIATE_BROKER_URL "$ENV{INITIATE_BROKER_URL}")
SET(INITIATE_MQTT_CLIENT_USERNAME "$ENV{INITIATE_MQTT_CLIENT_USERNAME}")
SET(INITIATE_MQTT_CLIENT_PASSWORD "$ENV{INITIATE_MQTT_CLIENT_USERNAME}")
# end of Mqtt STA Configuration

#
# SNTP configuration
#
SET(INITIATE_NTP_SERVER "$ENV{INITIATE_NTP_SERVER}")
SET(INITIATE_DEFAULT_LOCALE_TIME_ZONE "$ENV{INITIATE_DEFAULT_LOCALE_TIME_ZONE}")
configure_file(${CMAKE_CURRENT_LIST_DIR}/components/common/gen/default_initiate.in.h ${CMAKE_CURRENT_LIST_DIR}/components/common/include/default_initiate.h)


#
# Board pin map: hardware/boards/<BOARD>.cmake -> components/common/include/gpio_declaraion.h
#
set(BOARD "firebeetle2_c6" CACHE STRING "Board pin map in hardware/boards/")
include(${CMAKE_CURRENT_LIST_DIR}/hardware/boards/${BOARD}.cmake)
set(_pin_vars BOARD_PIN_LED BOARD_PIN_CONFIG_MODE BOARD_PIN_ONEWIRE BOARD_PIN_I2C_SDA BOARD_PIN_I2C_SCL
        BOARD_PIN_RS485_TX BOARD_PIN_RS485_RX BOARD_PIN_RS485_DE)
set(_used_pins "")
foreach (_var ${_pin_vars})
    if (${_var} IN_LIST _used_pins)
        message(WARNING "Board ${BOARD}: ${${_var}} assigned twice (${_var})")
    endif ()
    list(APPEND _used_pins ${${_var}})
endforeach ()
configure_file(${CMAKE_CURRENT_LIST_DIR}/components/common/gen/gpio_declaraion.in.h ${CMAKE_CURRENT_LIST_DIR}/components/common/include/gpio_declaraion.h @ONLY)
