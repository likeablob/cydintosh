set(UMAC_DIR "${CMAKE_SOURCE_DIR}/external/umac")
set(MUSASHI_DIR "${UMAC_DIR}/external/Musashi")

set(UMAC_SOURCES
    ${UMAC_DIR}/src/disc.c
    ${UMAC_DIR}/src/main.c
    ${UMAC_DIR}/src/rom.c
    ${UMAC_DIR}/src/scc.c
    ${UMAC_DIR}/src/via.c
)

set(MUSASHI_SOURCES
    ${MUSASHI_DIR}/m68kcpu.c
    ${MUSASHI_DIR}/m68kdasm.c
    ${MUSASHI_DIR}/m68kops.c
    ${MUSASHI_DIR}/softfloat/softfloat.c
)

idf_component_register(
    SRCS "main.c" "video.c" "display.c" "disc_lfs.c" "touch_cyd.c" "umac_ipc.c" "wifi_info.c" "mqtt_weather.c" "hw_control.c" ${UMAC_SOURCES} ${MUSASHI_SOURCES}
    INCLUDE_DIRS "." "${CMAKE_SOURCE_DIR}/include" "${UMAC_DIR}/include" "${MUSASHI_DIR}"
    REQUIRES driver esp_timer esp_lcd joltwallet__littlefs lcd_cyd esp_wifi esp_netif mqtt
)
