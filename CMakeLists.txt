# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)
set(IDF_TARGET esp32c6 CACHE STRING "cpuType" FORCE)
IF ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    ADD_COMPILE_DEFINITIONS(_DEBUG)
ENDIF ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(smart-irrigate
        LANGUAGES C CXX
)
SET(INITIATE_WIFI_SSID "$ENV{INITIATE_WIFI_SSID}")
SET(INITIATE_WIFI_PASSWORD "$ENV{INITIATE_WIFI_PASSWORD}")
# BT
SET(INITIATE_BT_DEVICE_NAME "$ENV{INITIATE_BT_DEVICE_NAME}")
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

