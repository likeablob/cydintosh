idf_component_register(SRCS "lcd_cyd.c"
                        INCLUDE_DIRS "include"
                        REQUIRES "esp_lcd_ili9341"
                        PRIV_REQUIRES "driver")