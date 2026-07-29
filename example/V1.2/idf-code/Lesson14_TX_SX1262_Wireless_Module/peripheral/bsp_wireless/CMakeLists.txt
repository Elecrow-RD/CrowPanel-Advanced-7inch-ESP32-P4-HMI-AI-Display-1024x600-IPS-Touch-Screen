FILE(GLOB_RECURSE component_sources "*.cpp")

idf_component_register(SRCS ${component_sources}
                        INCLUDE_DIRS "include"
                        REQUIRES driver esp_timer radiolib)
