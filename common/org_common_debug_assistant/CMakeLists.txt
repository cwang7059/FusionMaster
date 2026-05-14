add_library(org_common_debug_assistant SHARED
    src/debug_assistant_plugin.cpp
    src/debug_assistant_plugin.h
    src/debug_assistant_activator.cpp
    src/debug_assistant_activator.h
    resources/debug_assistant_manifest.qrc
)
set_property(GLOBAL APPEND PROPERTY OSGI_RUNTIME_PLUGIN_TARGETS org_common_debug_assistant)

if(NOT OSGI_CTK_AVAILABLE)
    message(FATAL_ERROR "org_common_debug_assistant 需要 CTK 运行时支持，请先准备 third_party/ctk")
endif()

file(GLOB DEBUG_ASSISTANT_CTK_PLUGINFW_LIB_CANDIDATES
    "${CTK_LIB_DIR}/*CTKPluginFramework*.lib"
    "${CTK_LIB_DIR}/*CTKPluginFramework*.a"
    "${CTK_LIB_DIR}/*CTKPluginFramework*.so"
    "${CTK_LIB_DIR}/*CTKPluginFramework*.so.*"
)

file(GLOB DEBUG_ASSISTANT_CTK_CORE_LIB_CANDIDATES
    "${CTK_LIB_DIR}/*CTKCore*.lib"
    "${CTK_LIB_DIR}/*CTKCore*.a"
    "${CTK_LIB_DIR}/*CTKCore*.so"
    "${CTK_LIB_DIR}/*CTKCore*.so.*"
)

list(LENGTH DEBUG_ASSISTANT_CTK_PLUGINFW_LIB_CANDIDATES DEBUG_ASSISTANT_CTK_PLUGINFW_LIB_COUNT)
list(LENGTH DEBUG_ASSISTANT_CTK_CORE_LIB_CANDIDATES DEBUG_ASSISTANT_CTK_CORE_LIB_COUNT)

if(DEBUG_ASSISTANT_CTK_PLUGINFW_LIB_COUNT EQUAL 0 OR DEBUG_ASSISTANT_CTK_CORE_LIB_COUNT EQUAL 0)
    message(FATAL_ERROR "未在 ${CTK_LIB_DIR} 找到 CTKPluginFramework/CTKCore 库文件")
endif()

list(GET DEBUG_ASSISTANT_CTK_PLUGINFW_LIB_CANDIDATES 0 DEBUG_ASSISTANT_CTK_PLUGINFW_LIB)
list(GET DEBUG_ASSISTANT_CTK_CORE_LIB_CANDIDATES 0 DEBUG_ASSISTANT_CTK_CORE_LIB)

set_target_properties(org_common_debug_assistant PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    OUTPUT_NAME "org_common_debug_assistant"
    DEBUG_POSTFIX "d"
)

target_include_directories(org_common_debug_assistant PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CTK_INCLUDE_DIR}/ctk-0.1
)

target_link_libraries(org_common_debug_assistant PRIVATE
    plugin_api
    ${OSGI_QT_CORE_TARGET}
    "${DEBUG_ASSISTANT_CTK_PLUGINFW_LIB}"
    "${DEBUG_ASSISTANT_CTK_CORE_LIB}"
)

if(OSGI_QT_CORE5COMPAT_TARGET)
    target_link_libraries(org_common_debug_assistant PRIVATE ${OSGI_QT_CORE5COMPAT_TARGET})
endif()

osgi_attach_qml_sources_to_current_target(org_common_debug_assistant)
