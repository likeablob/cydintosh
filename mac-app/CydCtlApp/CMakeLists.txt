cmake_minimum_required(VERSION 3.9)

project(CydCtl)

if(NOT CMAKE_TOOLCHAIN_FILE)
    message(FATAL_ERROR "CMAKE_TOOLCHAIN_FILE must be set to Retro68 toolchain")
endif()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../common common)

add_application(CydCtl
    CydCtlApp.c
    Resources.r
    TYPE "APPL"
    CREATOR "CYDC"
)

target_link_libraries(CydCtl esp_ipc)