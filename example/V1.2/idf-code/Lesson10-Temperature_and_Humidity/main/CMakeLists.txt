FILE(GLOB_RECURSE main ${CMAKE_SOURCE_DIR}/main/*.c)

idf_component_register(SRCS ${main}
                        INCLUDE_DIRS "include" 
                        REQUIRES bsp_i2c bsp_illuminate bsp_dht20)
                                 