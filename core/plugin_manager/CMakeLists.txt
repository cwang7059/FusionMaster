add_library(plugin_manager STATIC
    src/plugin_manager.cpp
    src/plugin_manager_bridge_service.h
)

set_target_properties(plugin_manager PROPERTIES
    AUTOMOC ON
)

target_include_directories(plugin_manager PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(plugin_manager PUBLIC plugin_api ${OSGI_QT_CORE_TARGET})

set(OSGI_CTK_RELEASE_AVAILABLE OFF)
set(OSGI_CTK_DEBUG_AVAILABLE OFF)
set(CTK_PLUGINFW_RELEASE_LIB "")
set(CTK_CORE_RELEASE_LIB "")
set(CTK_PLUGINFW_DEBUG_LIB "")
set(CTK_CORE_DEBUG_LIB "")

if(OSGI_CTK_AVAILABLE)
    target_include_directories(plugin_manager PRIVATE "${CTK_INCLUDE_DIR}/ctk-0.1")

    if(OSGI_QT_CORE5COMPAT_TARGET)
        target_link_libraries(plugin_manager PUBLIC ${OSGI_QT_CORE5COMPAT_TARGET})
    endif()

    file(GLOB CTK_PLUGINFW_LIB_CANDIDATES
        "${CTK_LIB_DIR}/*CTKPluginFramework*.lib"
        "${CTK_LIB_DIR}/*CTKPluginFramework*.a"
        "${CTK_LIB_DIR}/*CTKPluginFramework*.so"
        "${CTK_LIB_DIR}/*CTKPluginFramework*.so.*"
    )

    file(GLOB CTK_CORE_LIB_CANDIDATES
        "${CTK_LIB_DIR}/*CTKCore*.lib"
        "${CTK_LIB_DIR}/*CTKCore*.a"
        "${CTK_LIB_DIR}/*CTKCore*.so"
        "${CTK_LIB_DIR}/*CTKCore*.so.*"
    )

    foreach(_lib IN LISTS CTK_PLUGINFW_LIB_CANDIDATES)
        get_filename_component(_name "${_lib}" NAME)
        if(_name MATCHES "d\\.(lib|a|so)$" OR _name MATCHES "d\\.so\\.")
            if(CTK_PLUGINFW_DEBUG_LIB STREQUAL "")
                set(CTK_PLUGINFW_DEBUG_LIB "${_lib}")
            endif()
        else()
            if(CTK_PLUGINFW_RELEASE_LIB STREQUAL "")
                set(CTK_PLUGINFW_RELEASE_LIB "${_lib}")
            endif()
        endif()
    endforeach()

    foreach(_lib IN LISTS CTK_CORE_LIB_CANDIDATES)
        get_filename_component(_name "${_lib}" NAME)
        if(_name MATCHES "d\\.(lib|a|so)$" OR _name MATCHES "d\\.so\\.")
            if(CTK_CORE_DEBUG_LIB STREQUAL "")
                set(CTK_CORE_DEBUG_LIB "${_lib}")
            endif()
        else()
            if(CTK_CORE_RELEASE_LIB STREQUAL "")
                set(CTK_CORE_RELEASE_LIB "${_lib}")
            endif()
        endif()
    endforeach()

    if(NOT CTK_PLUGINFW_RELEASE_LIB OR NOT CTK_CORE_RELEASE_LIB)
        message(FATAL_ERROR "OSGI_CTK_AVAILABLE=ON 但未在 ${CTK_LIB_DIR} 找到 Release CTKPluginFramework/CTKCore 库文件")
    endif()

    set(OSGI_CTK_RELEASE_AVAILABLE ON)

    if(CTK_PLUGINFW_DEBUG_LIB AND CTK_CORE_DEBUG_LIB)
        set(OSGI_CTK_DEBUG_AVAILABLE ON)
    else()
        message(WARNING "未找到 Debug CTK 库，Debug 配置将禁用 CTK，Release 配置继续启用 CTK")
    endif()

    if(OSGI_CTK_DEBUG_AVAILABLE)
        target_link_libraries(plugin_manager PUBLIC
            "$<$<CONFIG:Debug>:${CTK_PLUGINFW_DEBUG_LIB}>"
            "$<$<CONFIG:Debug>:${CTK_CORE_DEBUG_LIB}>"
            "$<$<NOT:$<CONFIG:Debug>>:${CTK_PLUGINFW_RELEASE_LIB}>"
            "$<$<NOT:$<CONFIG:Debug>>:${CTK_CORE_RELEASE_LIB}>"
        )
    else()
        target_link_libraries(plugin_manager PUBLIC
            "$<$<NOT:$<CONFIG:Debug>>:${CTK_PLUGINFW_RELEASE_LIB}>"
            "$<$<NOT:$<CONFIG:Debug>>:${CTK_CORE_RELEASE_LIB}>"
        )
    endif()
endif()

if(OSGI_CTK_RELEASE_AVAILABLE)
    if(OSGI_CTK_DEBUG_AVAILABLE)
        target_compile_definitions(plugin_manager PUBLIC
            OSGI_CTK_AVAILABLE=1
        )
    else()
        target_compile_definitions(plugin_manager PUBLIC
            "$<$<CONFIG:Debug>:OSGI_CTK_AVAILABLE=0>"
            "$<$<NOT:$<CONFIG:Debug>>:OSGI_CTK_AVAILABLE=1>"
        )
    endif()
else()
    target_compile_definitions(plugin_manager PUBLIC
        OSGI_CTK_AVAILABLE=0
    )
endif()

target_compile_definitions(plugin_manager PRIVATE
    OSGI_PLATFORM_TAG="${OSGI_PLATFORM_TAG}"
    OSGI_PLATFORM_TAG_TOOLCHAIN="${OSGI_PLATFORM_TAG_TOOLCHAIN}"
)
