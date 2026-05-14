add_library(org_common_log_viewer SHARED
    src/log_viewer_plugin.cpp
    src/log_viewer_plugin.h
    src/log_viewer_controller.cpp
    src/log_viewer_controller.h
    src/log_viewer_activator.cpp
    src/log_viewer_activator.h
    resources/log_viewer_manifest.qrc
)
set_property(GLOBAL APPEND PROPERTY OSGI_RUNTIME_PLUGIN_TARGETS org_common_log_viewer)

if(NOT OSGI_CTK_AVAILABLE)
    message(FATAL_ERROR "org_common_log_viewer 闇€瑕?CTK 杩愯鏃舵敮鎸侊紝璇峰厛鍑嗗 third_party/ctk")
endif()

file(GLOB LOG_VIEWER_CTK_PLUGINFW_LIB_CANDIDATES
    "${CTK_LIB_DIR}/*CTKPluginFramework*.lib"
    "${CTK_LIB_DIR}/*CTKPluginFramework*.a"
    "${CTK_LIB_DIR}/*CTKPluginFramework*.so"
    "${CTK_LIB_DIR}/*CTKPluginFramework*.so.*"
)

file(GLOB LOG_VIEWER_CTK_CORE_LIB_CANDIDATES
    "${CTK_LIB_DIR}/*CTKCore*.lib"
    "${CTK_LIB_DIR}/*CTKCore*.a"
    "${CTK_LIB_DIR}/*CTKCore*.so"
    "${CTK_LIB_DIR}/*CTKCore*.so.*"
)

list(LENGTH LOG_VIEWER_CTK_PLUGINFW_LIB_CANDIDATES LOG_VIEWER_CTK_PLUGINFW_LIB_COUNT)
list(LENGTH LOG_VIEWER_CTK_CORE_LIB_CANDIDATES LOG_VIEWER_CTK_CORE_LIB_COUNT)

if(LOG_VIEWER_CTK_PLUGINFW_LIB_COUNT EQUAL 0 OR LOG_VIEWER_CTK_CORE_LIB_COUNT EQUAL 0)
    message(FATAL_ERROR "未在 ${CTK_LIB_DIR} 找到 CTKPluginFramework/CTKCore 库文件")
endif()

list(GET LOG_VIEWER_CTK_PLUGINFW_LIB_CANDIDATES 0 LOG_VIEWER_CTK_PLUGINFW_LIB)
list(GET LOG_VIEWER_CTK_CORE_LIB_CANDIDATES 0 LOG_VIEWER_CTK_CORE_LIB)

set_target_properties(org_common_log_viewer PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    OUTPUT_NAME "org_common_log_viewer"
    DEBUG_POSTFIX "d"
)

target_include_directories(org_common_log_viewer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CTK_INCLUDE_DIR}/ctk-0.1
)

target_link_libraries(org_common_log_viewer PRIVATE
    plugin_api
    logging
    ${OSGI_QT_CORE_TARGET}
    ${OSGI_QT_WIDGETS_TARGET}
    "${LOG_VIEWER_CTK_PLUGINFW_LIB}"
    "${LOG_VIEWER_CTK_CORE_LIB}"
)

if(OSGI_QT_CORE5COMPAT_TARGET)
    target_link_libraries(org_common_log_viewer PRIVATE ${OSGI_QT_CORE5COMPAT_TARGET})
endif()

osgi_attach_qml_sources_to_current_target(org_common_log_viewer)
