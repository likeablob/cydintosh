add_library(esp_ipc STATIC esp_ipc.c)
target_include_directories(esp_ipc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})