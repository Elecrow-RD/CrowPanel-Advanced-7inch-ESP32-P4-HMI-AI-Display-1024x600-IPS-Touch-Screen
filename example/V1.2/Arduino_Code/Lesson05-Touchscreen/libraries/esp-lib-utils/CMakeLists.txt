set(SRC_DIR ./src)
file(GLOB_RECURSE SRCS_C ${SRC_DIR}/*.c)
file(GLOB_RECURSE SRCS_CPP ${SRC_DIR}/*.cpp)

if(ESP_PLATFORM)
    idf_component_register(
        SRCS ${SRCS_C} ${SRCS_CPP}
        INCLUDE_DIRS ${SRC_DIR}
        PRIV_REQUIRES pthread
    )
else()
    set(COMPONENT_LIB esp-lib-utils)
    add_library(${COMPONENT_LIB} STATIC ${SRCS_C} ${SRCS_CPP})
    target_include_directories(${COMPONENT_LIB} PUBLIC ${SRC_DIR})
endif()

target_compile_options(${COMPONENT_LIB}
    PUBLIC
        -Wno-missing-field-initializers
)

if(NOT ESP_PLATFORM)
    target_compile_definitions(${COMPONENT_LIB} PUBLIC ESP_UTILS_KCONFIG_IGNORE)
endif()
