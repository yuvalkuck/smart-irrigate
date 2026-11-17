idf_component_register(SRCS "hwifi_sta.cpp" "soft_ap.cpp" "ap_webserver.cpp"
        PRIV_REQUIRES log esp_wifi esp_netif mqtt esp_http_server espressif__fmt
        REQUIRES flash common
        INCLUDE_DIRS "include")

# mod_types_gen.h: the table behind the GET /api/status JSON (what the iSetup app may modify).
# It is generated at configure time, not committed, from:
#   - flash.h                         NVS keys (SETUP_KEY_NAME), enum entries and their bit shifts
#   - protocol/schema/mod_types.yaml  titles, defaults and ranges that flash.h cannot give
#   - common/gen/mod_types.in.h       the template the table is written into
# The script also syncs the yaml with flash.h in place: it adds entries for a new key or enum
# entry, removes the ones that are gone, and keeps everything else (run it with -h for details).

# the Python of the ESP-IDF environment, it has the ruamel.yaml package the script needs
idf_build_get_property(python PYTHON)
set(MOD_TYPES_SCRIPT_DIR ${CMAKE_CURRENT_LIST_DIR}/../protocol/script)
set(MOD_TYPES_TEMPLATE ${CMAKE_CURRENT_LIST_DIR}/../common/gen/mod_types.in.h)
set(MOD_TYPES_YAML ${CMAKE_CURRENT_LIST_DIR}/../protocol/schema/mod_types.yaml)
set(MOD_TYPES_FLASH_H ${CMAKE_CURRENT_LIST_DIR}/../flash/include/flash.h)
# the output goes to the build directory, so the source tree stays clean
set(MOD_TYPES_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/gen)
file(MAKE_DIRECTORY ${MOD_TYPES_GEN_DIR})
# run the generator now, so the header exists before any source is compiled
execute_process(
        COMMAND ${python} ${MOD_TYPES_SCRIPT_DIR}/gen_mod_types.py
                --flash-h ${MOD_TYPES_FLASH_H} --yaml ${MOD_TYPES_YAML}
                --template ${MOD_TYPES_TEMPLATE}
                --out ${MOD_TYPES_GEN_DIR}/mod_types_gen.h
        RESULT_VARIABLE MOD_TYPES_RESULT
        ERROR_VARIABLE MOD_TYPES_ERROR)
# a mismatch the script cannot fix (e.g. a section whose enum is gone from flash.h) stops the build
if(NOT MOD_TYPES_RESULT EQUAL 0)
    message(FATAL_ERROR "${MOD_TYPES_ERROR}")
endif()
# configure again, and so regenerate, when any input of the script changes
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
        ${MOD_TYPES_YAML} ${MOD_TYPES_FLASH_H}
        ${MOD_TYPES_SCRIPT_DIR}/gen_mod_types.py ${MOD_TYPES_TEMPLATE})
# ap_webserver.cpp includes "mod_types_gen.h" from there; no other component sees it
target_include_directories(${COMPONENT_LIB} PRIVATE ${MOD_TYPES_GEN_DIR})

target_compile_options(${COMPONENT_LIB} PRIVATE -std=gnu++20 -fno-exceptions -fno-rtti)