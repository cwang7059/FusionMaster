set(COMMON_PLUGIN_FILTER "*" CACHE STRING "Common plugins to build, '*' or ';' separated names")

if(COMMON_PLUGIN_FILTER STREQUAL "*")
    set(_common_filter_all TRUE)
else()
    set(_common_filter_all FALSE)
    separate_arguments(COMMON_PLUGIN_FILTER_LIST WINDOWS_COMMAND "${COMMON_PLUGIN_FILTER}")
endif()

# 三段式命名: org_platform_plugin
set(OSGI_PLUGIN_NAME_PATTERN "^[a-z][a-z0-9]*_[a-z][a-z0-9]*_[a-z][a-z0-9_]*$")

file(GLOB common_plugin_dirs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *)
foreach(dir ${common_plugin_dirs})
    if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${dir}
       AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${dir}/CMakeLists.txt)
        if(NOT dir MATCHES "${OSGI_PLUGIN_NAME_PATTERN}")
            message(FATAL_ERROR "插件目录命名不符合三段式规范: ${dir}，示例: org_common_debug_assistant")
        endif()

        if(dir STREQUAL "org_common_debug_assistant" AND NOT ENABLE_DEBUG_ASSISTANT)
            message(STATUS "common: 调试助手已关闭，跳过 ${dir}")
            continue()
        endif()

        if(_common_filter_all)
            add_subdirectory(${dir})
        else()
            list(FIND COMMON_PLUGIN_FILTER_LIST ${dir} hit_index)
            if(NOT hit_index EQUAL -1)
                add_subdirectory(${dir})
            endif()
        endif()
    endif()
endforeach()
