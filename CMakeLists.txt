# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(wifi_provisioning_new C CXX ASM)


# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add libary
add_library(wifi_prov_lib INTERFACE)
target_sources(wifi_prov_lib INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/wifi_prov_lib.c
        )
target_include_directories(wifi_prov_lib INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}
        )
target_link_libraries(wifi_prov_lib INTERFACE
    pico_stdlib
    pico_btstack_ble
    pico_btstack_cyw43
    pico_cyw43_arch_lwip_threadsafe_background
    hardware_flash
        )
pico_btstack_make_gatt_header(wifi_prov_lib INTERFACE "${CMAKE_CURRENT_LIST_DIR}/provisioning.gatt")

# Add executable. Default name is the project name, version 0.1
# Standalone example that reads from the on board temperature sensor and sends notifications via BLE
# Flashes slowly each second to show it's running
add_executable(wifi_provisioning_new
    example.c 
    )
target_link_libraries(wifi_provisioning_new
    pico_stdlib
    wifi_prov_lib
    pico_lwip_iperf
    )
target_include_directories(wifi_provisioning_new PRIVATE
    ${CMAKE_CURRENT_LIST_DIR} # For btstack config
    )

pico_add_extra_outputs(wifi_provisioning_new)

