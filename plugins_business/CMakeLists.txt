set(BUSINESS_PLUGIN_FILTER "*" CACHE STRING "Business plugins to build, '*' or ';' separated names")

if(BUSINESS_PLUGIN_FILTER STREQUAL "*")
    set(_business_filter_all TRUE)
else()
    set(_business_filter_all FALSE)
    separate_arguments(BUSINESS_PLUGIN_FILTER_LIST WINDOWS_COMMAND "${BUSINESS_PLUGIN_FILTER}")
endif()

# 三段式命名: org_platform_plugin
set(OSGI_PLUGIN_NAME_PATTERN "^[a-z][a-z0-9]*_[a-z][a-z0-9]*_[a-z][a-z0-9_]*$")

file(GLOB plugin_dirs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *)
foreach(dir ${plugin_dirs})
    if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${dir}
       AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${dir}/CMakeLists.txt)
        if(NOT dir MATCHES "${OSGI_PLUGIN_NAME_PATTERN}")
            message(FATAL_ERROR "业务插件目录命名不符合三段式规范: ${dir}，示例: org_business_data_display")
        endif()

        if(_business_filter_all)
            add_subdirectory(${dir})
        else()
            list(FIND BUSINESS_PLUGIN_FILTER_LIST ${dir} hit_index)
            if(NOT hit_index EQUAL -1)
                add_subdirectory(${dir})
            endif()
        endif()
    endif()
endforeach()
