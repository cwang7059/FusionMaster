add_library(org_common_encoder_panel SHARED
    src/encoder_panel_plugin.cpp
    src/encoder_panel_plugin.h
    src/encoder_panel_activator.cpp
    src/encoder_panel_activator.h
    resources/encoder_panel_manifest.qrc
)
set_property(GLOBAL APPEND PROPERTY OSGI_RUNTIME_PLUGIN_TARGETS org_common_encoder_panel)

if(NOT OSGI_CTK_AVAILABLE)
    message(FATAL_ERROR "org_common_encoder_panel 需要 CTK 运行时支持，请先准备 third_party/ctk")
endif()

file(GLOB ENCODER_PANEL_CTK_PLUGINFW_LIB_CANDIDATES
    "${CTK_LIB_DIR}/*CTKPluginFramework*.lib"
    "${CTK_LIB_DIR}/*CTKPluginFramework*.a"
    "${CTK_LIB_DIR}/*CTKPluginFramework*.so"
    "${CTK_LIB_DIR}/*CTKPluginFramework*.so.*"
)

file(GLOB ENCODER_PANEL_CTK_CORE_LIB_CANDIDATES
    "${CTK_LIB_DIR}/*CTKCore*.lib"
    "${CTK_LIB_DIR}/*CTKCore*.a"
    "${CTK_LIB_DIR}/*CTKCore*.so"
    "${CTK_LIB_DIR}/*CTKCore*.so.*"
)

list(LENGTH ENCODER_PANEL_CTK_PLUGINFW_LIB_CANDIDATES ENCODER_PANEL_CTK_PLUGINFW_LIB_COUNT)
list(LENGTH ENCODER_PANEL_CTK_CORE_LIB_CANDIDATES ENCODER_PANEL_CTK_CORE_LIB_COUNT)

if(ENCODER_PANEL_CTK_PLUGINFW_LIB_COUNT EQUAL 0 OR ENCODER_PANEL_CTK_CORE_LIB_COUNT EQUAL 0)
    message(FATAL_ERROR "未在 ${CTK_LIB_DIR} 找到 CTKPluginFramework/CTKCore 库文件")
endif()

list(GET ENCODER_PANEL_CTK_PLUGINFW_LIB_CANDIDATES 0 ENCODER_PANEL_CTK_PLUGINFW_LIB)
list(GET ENCODER_PANEL_CTK_CORE_LIB_CANDIDATES 0 ENCODER_PANEL_CTK_CORE_LIB)

set_target_properties(org_common_encoder_panel PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    OUTPUT_NAME "org_common_encoder_panel"
    DEBUG_POSTFIX "d"
)

target_include_directories(org_common_encoder_panel PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CTK_INCLUDE_DIR}/ctk-0.1
)

target_link_libraries(org_common_encoder_panel PRIVATE
    plugin_api
    ${OSGI_QT_CORE_TARGET}
    "${ENCODER_PANEL_CTK_PLUGINFW_LIB}"
    "${ENCODER_PANEL_CTK_CORE_LIB}"
)

if(OSGI_QT_CORE5COMPAT_TARGET)
    target_link_libraries(org_common_encoder_panel PRIVATE ${OSGI_QT_CORE5COMPAT_TARGET})
endif()

osgi_attach_qml_sources_to_current_target(org_common_encoder_panel)
