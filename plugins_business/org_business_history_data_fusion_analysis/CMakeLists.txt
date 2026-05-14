add_library(org_business_history_data_fusion_analysis SHARED
    src/history_data_fusion_analysis_activator.cpp
    src/history_data_fusion_analysis_activator.h
    src/history_data_fusion_analysis_plugin.cpp
    src/history_data_fusion_analysis_plugin.h
    src/history_data_fusion_controller.cpp
    src/history_data_fusion_controller.h
    resources/history_data_fusion_analysis_manifest.qrc
)
set_property(GLOBAL APPEND PROPERTY OSGI_RUNTIME_PLUGIN_TARGETS org_business_history_data_fusion_analysis)

if(NOT OSGI_CTK_AVAILABLE)
    message(FATAL_ERROR "org_business_history_data_fusion_analysis requires CTK runtime support")
endif()

file(GLOB HISTORY_DATA_FUSION_CTK_PLUGINFW_LIB_CANDIDATES
    "${CTK_LIB_DIR}/*CTKPluginFramework*.lib"
    "${CTK_LIB_DIR}/*CTKPluginFramework*.a"
    "${CTK_LIB_DIR}/*CTKPluginFramework*.so"
    "${CTK_LIB_DIR}/*CTKPluginFramework*.so.*"
)

file(GLOB HISTORY_DATA_FUSION_CTK_CORE_LIB_CANDIDATES
    "${CTK_LIB_DIR}/*CTKCore*.lib"
    "${CTK_LIB_DIR}/*CTKCore*.a"
    "${CTK_LIB_DIR}/*CTKCore*.so"
    "${CTK_LIB_DIR}/*CTKCore*.so.*"
)

list(LENGTH HISTORY_DATA_FUSION_CTK_PLUGINFW_LIB_CANDIDATES HISTORY_DATA_FUSION_CTK_PLUGINFW_LIB_COUNT)
list(LENGTH HISTORY_DATA_FUSION_CTK_CORE_LIB_CANDIDATES HISTORY_DATA_FUSION_CTK_CORE_LIB_COUNT)

if(HISTORY_DATA_FUSION_CTK_PLUGINFW_LIB_COUNT EQUAL 0 OR HISTORY_DATA_FUSION_CTK_CORE_LIB_COUNT EQUAL 0)
    message(FATAL_ERROR "CTKPluginFramework/CTKCore libraries were not found in ${CTK_LIB_DIR}")
endif()

list(GET HISTORY_DATA_FUSION_CTK_PLUGINFW_LIB_CANDIDATES 0 HISTORY_DATA_FUSION_CTK_PLUGINFW_LIB)
list(GET HISTORY_DATA_FUSION_CTK_CORE_LIB_CANDIDATES 0 HISTORY_DATA_FUSION_CTK_CORE_LIB)

set_target_properties(org_business_history_data_fusion_analysis PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    OUTPUT_NAME "org_business_history_data_fusion_analysis"
    DEBUG_POSTFIX "d"
)

target_include_directories(org_business_history_data_fusion_analysis PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CTK_INCLUDE_DIR}/ctk-0.1
)

target_link_libraries(org_business_history_data_fusion_analysis PRIVATE
    plugin_api
    ${OSGI_QT_CORE_TARGET}
    ${OSGI_QT_WIDGETS_TARGET}
    ${OSGI_QT_QML_TARGET}
    "${HISTORY_DATA_FUSION_CTK_PLUGINFW_LIB}"
    "${HISTORY_DATA_FUSION_CTK_CORE_LIB}"
)

if(OSGI_QT_CORE5COMPAT_TARGET)
    target_link_libraries(org_business_history_data_fusion_analysis PRIVATE ${OSGI_QT_CORE5COMPAT_TARGET})
endif()

osgi_attach_qml_sources_to_current_target(org_business_history_data_fusion_analysis)
