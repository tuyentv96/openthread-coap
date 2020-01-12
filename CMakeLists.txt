# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(coap_client)

if(CONFIG_MBEDTLS_KEY_EXCHANGE_PSK_ENABLED AND
    (CONFIG_NET_SAMPLE_PSK_HEADER_FILE STREQUAL "dummy_psk.h"))
  add_custom_target(development_psk
    COMMAND ${CMAKE_COMMAND} -E echo "----------------------------------------------------------"
    COMMAND ${CMAKE_COMMAND} -E echo "--- WARNING: Using dummy PSK! Only suitable for        ---"
    COMMAND ${CMAKE_COMMAND} -E echo "--- development. Set NET_SAMPLE_PSK_HEADER_FILE to use ---"
    COMMAND ${CMAKE_COMMAND} -E echo "--- own pre-shared key.                                ---"
    COMMAND ${CMAKE_COMMAND} -E echo "----------------------------------------------------------"
  )
  add_dependencies(app development_psk)
endif()

FILE(GLOB utils_sources src/utils/*.c)
FILE(GLOB hal_sources src/hal/*.c)
FILE(GLOB drivers_sources src/drivers/*.c)
FILE(GLOB app_sources src/app/*.c)

target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE ${hal_sources})
target_sources(app PRIVATE ${drivers_sources})
target_sources(app PRIVATE ${app_sources})
target_sources(app PRIVATE ${utils_sources})

target_include_directories(app PRIVATE $ENV{ZEPHYR_BASE}/subsys/net/ip)
target_include_directories(app PRIVATE include)


include($ENV{ZEPHYR_BASE}/samples/net/common/common.cmake)

set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)

generate_inc_file_for_target(
    app
    src/echo-apps-cert.der
    ${gen_dir}/echo-apps-cert.der.inc
    )
