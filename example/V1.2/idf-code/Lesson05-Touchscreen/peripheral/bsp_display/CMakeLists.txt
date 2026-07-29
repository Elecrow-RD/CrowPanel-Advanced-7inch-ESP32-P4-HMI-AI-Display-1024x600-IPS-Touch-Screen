FILE(GLOB_RECURSE component_sources "*.c")

idf_component_register(SRCS ${component_sources}
                        INCLUDE_DIRS "include"
                        REQUIRES esp_lcd_touch_gt911 bsp_i2c)


