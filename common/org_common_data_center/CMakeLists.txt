add_library(org_common_data_center SHARED
    src/data_center_plugin.cpp
    src/data_center_plugin.h
    src/data_center_activator.cpp
    src/data_center_activator.h
    src/data_model_center_service.cpp
    src/data_model_center_service.h
    resources/data_center_manifest.qrc
)
set_property(GLOBAL APPEND PROPERTY OSGI_RUNTIME_PLUGIN_TARGETS org_common_data_center)

if(NOT OSGI_CTK_AVAILABLE)
    message(FATAL_ERROR "org_common_data_center 闇€瑕?CTK 杩愯鏃舵敮鎸侊紝璇峰厛鍑嗗 third_party/ctk")
endif()

file(GLOB DATA_CENTER_CTK_PLUGINFW_LIB_CANDIDATES
    "${CTK_LIB_DIR}/*CTKPluginFramework*.lib"
    "${CTK_LIB_DIR}/*CTKPluginFramework*.a"
    "${CTK_LIB_DIR}/*CTKPluginFramework*.so"
    "${CTK_LIB_DIR}/*CTKPluginFramework*.so.*"
)

file(GLOB DATA_CENTER_CTK_CORE_LIB_CANDIDATES
    "${CTK_LIB_DIR}/*CTKCore*.lib"
    "${CTK_LIB_DIR}/*CTKCore*.a"
    "${CTK_LIB_DIR}/*CTKCore*.so"
    "${CTK_LIB_DIR}/*CTKCore*.so.*"
)

list(LENGTH DATA_CENTER_CTK_PLUGINFW_LIB_CANDIDATES DATA_CENTER_CTK_PLUGINFW_LIB_COUNT)
list(LENGTH DATA_CENTER_CTK_CORE_LIB_CANDIDATES DATA_CENTER_CTK_CORE_LIB_COUNT)

if(DATA_CENTER_CTK_PLUGINFW_LIB_COUNT EQUAL 0 OR DATA_CENTER_CTK_CORE_LIB_COUNT EQUAL 0)
    message(FATAL_ERROR "未在 ${CTK_LIB_DIR} 找到 CTKPluginFramework/CTKCore 库文件")
endif()

list(GET DATA_CENTER_CTK_PLUGINFW_LIB_CANDIDATES 0 DATA_CENTER_CTK_PLUGINFW_LIB)
list(GET DATA_CENTER_CTK_CORE_LIB_CANDIDATES 0 DATA_CENTER_CTK_CORE_LIB)

set_target_properties(org_common_data_center PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    OUTPUT_NAME "org_common_data_center"
    DEBUG_POSTFIX "d"
)

target_include_directories(org_common_data_center PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CTK_INCLUDE_DIR}/ctk-0.1
)

target_link_libraries(org_common_data_center PRIVATE
    plugin_api
    ${OSGI_QT_CORE_TARGET}
    "${DATA_CENTER_CTK_PLUGINFW_LIB}"
    "${DATA_CENTER_CTK_CORE_LIB}"
)

if(OSGI_QT_CORE5COMPAT_TARGET)
    target_link_libraries(org_common_data_center PRIVATE ${OSGI_QT_CORE5COMPAT_TARGET})
endif()
