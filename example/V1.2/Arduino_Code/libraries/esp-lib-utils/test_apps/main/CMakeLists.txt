idf_component_register(
    SRCS "test_app_main.cpp" "test_on_c.c" "test_on_cpp.cpp"
    WHOLE_ARCHIVE
)

# Function to get component library
function(get_component_library component_name output_var)
    # Get the exact component name
    idf_build_get_property(build_components BUILD_COMPONENTS)
    set(TARGET_COMPONENT "")
    foreach(COMPONENT ${build_components})
        if(COMPONENT MATCHES "${component_name}" OR COMPONENT MATCHES "espressif__${component_name}")
            set(TARGET_COMPONENT ${COMPONENT})
            break()
        endif()
    endforeach()

    # Get the component library
    if(TARGET_COMPONENT STREQUAL "")
        message(FATAL_ERROR "Component '${component_name}' not found.")
    else()
        idf_component_get_property(COMPONENT_LIB ${TARGET_COMPONENT} COMPONENT_LIB)
        set(${output_var} ${COMPONENT_LIB} PARENT_SCOPE)
    endif()
endfunction()

get_component_library("esp-lib-utils" ESP_LIB_UTILS_LIB)
if(CONFIG_ESP_UTILS_CONF_MEM_GEN_ALLOC_TYPE_CUSTOM)
    target_compile_options(
        ${ESP_LIB_UTILS_LIB}
        PUBLIC
            "-DESP_UTILS_CONF_MEM_GEN_ALLOC_CUSTOM_INCLUDE=\"esp_heap_caps.h\""
    )
    target_compile_options(
        ${ESP_LIB_UTILS_LIB}
        PUBLIC
            "-DESP_UTILS_CONF_MEM_GEN_ALLOC_CUSTOM_MALLOC(x)=heap_caps_aligned_alloc(1, x, MALLOC_CAP_DEFAULT | MALLOC_CAP_8BIT)"
    )
    target_compile_options(
        ${ESP_LIB_UTILS_LIB}
        PUBLIC
            "-DESP_UTILS_CONF_MEM_GEN_ALLOC_CUSTOM_FREE(x)=heap_caps_free(x)"
    )
endif()
if(CONFIG_ESP_UTILS_CONF_MEM_CXX_GLOB_ALLOC_TYPE_CUSTOM)
    target_compile_options(
        ${ESP_LIB_UTILS_LIB}
        PUBLIC
            "-DESP_UTILS_CONF_MEM_CXX_GLOB_ALLOC_CUSTOM_INCLUDE=\"esp_heap_caps.h\""
    )
    target_compile_options(
        ${ESP_LIB_UTILS_LIB}
        PUBLIC
            "-DESP_UTILS_CONF_MEM_CXX_GLOB_ALLOC_CUSTOM_NEW(x)=heap_caps_aligned_alloc(1, x, MALLOC_CAP_DEFAULT | MALLOC_CAP_8BIT)"
    )
    target_compile_options(
        ${ESP_LIB_UTILS_LIB}
        PUBLIC
            "-DESP_UTILS_CONF_MEM_CXX_GLOB_ALLOC_CUSTOM_DELETE(x)=heap_caps_free(x)"
    )
endif()
