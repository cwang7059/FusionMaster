add_library(org_common_eddy_current_panel SHARED
    src/eddy_current_panel_plugin.cpp
    src/eddy_current_panel_plugin.h
    src/eddy_current_panel_activator.cpp
    src/eddy_current_panel_activator.h
    resources/eddy_current_panel_manifest.qrc
)
set_property(GLOBAL APPEND PROPERTY OSGI_RUNTIME_PLUGIN_TARGETS org_common_eddy_current_panel)

if(NOT OSGI_CTK_AVAILABLE)
    message(FATAL_ERROR "org_common_eddy_current_panel requires CTK runtime support, please prepare third_party/ctk first")
endif()

file(GLOB EDDY_CURRENT_PANEL_CTK_PLUGINFW_LIB_CANDIDATES
    "${CTK_LIB_DIR}/*CTKPluginFramework*.lib"
    "${CTK_LIB_DIR}/*CTKPluginFramework*.a"
    "${CTK_LIB_DIR}/*CTKPluginFramework*.so"
    "${CTK_LIB_DIR}/*CTKPluginFramework*.so.*"
)

file(GLOB EDDY_CURRENT_PANEL_CTK_CORE_LIB_CANDIDATES
    "${CTK_LIB_DIR}/*CTKCore*.lib"
    "${CTK_LIB_DIR}/*CTKCore*.a"
    "${CTK_LIB_DIR}/*CTKCore*.so"
    "${CTK_LIB_DIR}/*CTKCore*.so.*"
)

list(LENGTH EDDY_CURRENT_PANEL_CTK_PLUGINFW_LIB_CANDIDATES EDDY_CURRENT_PANEL_CTK_PLUGINFW_LIB_COUNT)
list(LENGTH EDDY_CURRENT_PANEL_CTK_CORE_LIB_CANDIDATES EDDY_CURRENT_PANEL_CTK_CORE_LIB_COUNT)

if(EDDY_CURRENT_PANEL_CTK_PLUGINFW_LIB_COUNT EQUAL 0 OR EDDY_CURRENT_PANEL_CTK_CORE_LIB_COUNT EQUAL 0)
    message(FATAL_ERROR "CTKPluginFramework/CTKCore libraries were not found under ${CTK_LIB_DIR}")
endif()

list(GET EDDY_CURRENT_PANEL_CTK_PLUGINFW_LIB_CANDIDATES 0 EDDY_CURRENT_PANEL_CTK_PLUGINFW_LIB)
list(GET EDDY_CURRENT_PANEL_CTK_CORE_LIB_CANDIDATES 0 EDDY_CURRENT_PANEL_CTK_CORE_LIB)

set_target_properties(org_common_eddy_current_panel PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    OUTPUT_NAME "org_common_eddy_current_panel"
    DEBUG_POSTFIX "d"
)

target_include_directories(org_common_eddy_current_panel PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CTK_INCLUDE_DIR}/ctk-0.1
)

target_link_libraries(org_common_eddy_current_panel PRIVATE
    plugin_api
    ${OSGI_QT_CORE_TARGET}
    "${EDDY_CURRENT_PANEL_CTK_PLUGINFW_LIB}"
    "${EDDY_CURRENT_PANEL_CTK_CORE_LIB}"
)

if(OSGI_QT_CORE5COMPAT_TARGET)
    target_link_libraries(org_common_eddy_current_panel PRIVATE ${OSGI_QT_CORE5COMPAT_TARGET})
endif()

osgi_attach_qml_sources_to_current_target(org_common_eddy_current_panel)
