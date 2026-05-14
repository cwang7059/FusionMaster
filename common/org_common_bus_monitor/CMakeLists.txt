add_library(org_common_bus_monitor SHARED
    src/bus_monitor_plugin.cpp
    src/bus_monitor_plugin.h
    src/bus_monitor_controller.cpp
    src/bus_monitor_controller.h
    src/bus_monitor_activator.cpp
    src/bus_monitor_activator.h
    resources/bus_monitor_manifest.qrc
)
set_property(GLOBAL APPEND PROPERTY OSGI_RUNTIME_PLUGIN_TARGETS org_common_bus_monitor)

if(NOT OSGI_CTK_AVAILABLE)
    message(FATAL_ERROR "org_common_bus_monitor 闇€瑕?CTK 杩愯鏃舵敮鎸侊紝璇峰厛鍑嗗 third_party/ctk")
endif()

file(GLOB BUS_MONITOR_CTK_PLUGINFW_LIB_CANDIDATES
    "${CTK_LIB_DIR}/*CTKPluginFramework*.lib"
    "${CTK_LIB_DIR}/*CTKPluginFramework*.a"
    "${CTK_LIB_DIR}/*CTKPluginFramework*.so"
    "${CTK_LIB_DIR}/*CTKPluginFramework*.so.*"
)

file(GLOB BUS_MONITOR_CTK_CORE_LIB_CANDIDATES
    "${CTK_LIB_DIR}/*CTKCore*.lib"
    "${CTK_LIB_DIR}/*CTKCore*.a"
    "${CTK_LIB_DIR}/*CTKCore*.so"
    "${CTK_LIB_DIR}/*CTKCore*.so.*"
)

list(LENGTH BUS_MONITOR_CTK_PLUGINFW_LIB_CANDIDATES BUS_MONITOR_CTK_PLUGINFW_LIB_COUNT)
list(LENGTH BUS_MONITOR_CTK_CORE_LIB_CANDIDATES BUS_MONITOR_CTK_CORE_LIB_COUNT)

if(BUS_MONITOR_CTK_PLUGINFW_LIB_COUNT EQUAL 0 OR BUS_MONITOR_CTK_CORE_LIB_COUNT EQUAL 0)
    message(FATAL_ERROR "未在 ${CTK_LIB_DIR} 找到 CTKPluginFramework/CTKCore 库文件")
endif()

list(GET BUS_MONITOR_CTK_PLUGINFW_LIB_CANDIDATES 0 BUS_MONITOR_CTK_PLUGINFW_LIB)
list(GET BUS_MONITOR_CTK_CORE_LIB_CANDIDATES 0 BUS_MONITOR_CTK_CORE_LIB)

set_target_properties(org_common_bus_monitor PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    OUTPUT_NAME "org_common_bus_monitor"
    DEBUG_POSTFIX "d"
)

target_include_directories(org_common_bus_monitor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CTK_INCLUDE_DIR}/ctk-0.1
)

target_link_libraries(org_common_bus_monitor PRIVATE
    plugin_api
    ${OSGI_QT_CORE_TARGET}
    ${OSGI_QT_WIDGETS_TARGET}
    "${BUS_MONITOR_CTK_PLUGINFW_LIB}"
    "${BUS_MONITOR_CTK_CORE_LIB}"
)

if(OSGI_QT_CORE5COMPAT_TARGET)
    target_link_libraries(org_common_bus_monitor PRIVATE ${OSGI_QT_CORE5COMPAT_TARGET})
endif()

osgi_attach_qml_sources_to_current_target(org_common_bus_monitor)
