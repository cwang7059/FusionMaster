add_library(org_common_net_receiver SHARED
    src/net_receiver_plugin.cpp
    src/net_receiver_plugin.h
    src/net_receiver_activator.cpp
    src/net_receiver_activator.h
    resources/net_receiver_manifest.qrc
)
set_property(GLOBAL APPEND PROPERTY OSGI_RUNTIME_PLUGIN_TARGETS org_common_net_receiver)

if(NOT OSGI_CTK_AVAILABLE)
    message(FATAL_ERROR "org_common_net_receiver 需要 CTK 运行时支持，请先准备 third_party/ctk")
endif()

file(GLOB NET_RECEIVER_CTK_PLUGINFW_LIB_CANDIDATES
    "${CTK_LIB_DIR}/*CTKPluginFramework*.lib"
    "${CTK_LIB_DIR}/*CTKPluginFramework*.a"
    "${CTK_LIB_DIR}/*CTKPluginFramework*.so"
    "${CTK_LIB_DIR}/*CTKPluginFramework*.so.*"
)

file(GLOB NET_RECEIVER_CTK_CORE_LIB_CANDIDATES
    "${CTK_LIB_DIR}/*CTKCore*.lib"
    "${CTK_LIB_DIR}/*CTKCore*.a"
    "${CTK_LIB_DIR}/*CTKCore*.so"
    "${CTK_LIB_DIR}/*CTKCore*.so.*"
)

list(LENGTH NET_RECEIVER_CTK_PLUGINFW_LIB_CANDIDATES NET_RECEIVER_CTK_PLUGINFW_LIB_COUNT)
list(LENGTH NET_RECEIVER_CTK_CORE_LIB_CANDIDATES NET_RECEIVER_CTK_CORE_LIB_COUNT)

if(NET_RECEIVER_CTK_PLUGINFW_LIB_COUNT EQUAL 0 OR NET_RECEIVER_CTK_CORE_LIB_COUNT EQUAL 0)
    message(FATAL_ERROR "未在 ${CTK_LIB_DIR} 找到 CTKPluginFramework/CTKCore 库文件")
endif()

list(GET NET_RECEIVER_CTK_PLUGINFW_LIB_CANDIDATES 0 NET_RECEIVER_CTK_PLUGINFW_LIB)
list(GET NET_RECEIVER_CTK_CORE_LIB_CANDIDATES 0 NET_RECEIVER_CTK_CORE_LIB)

set_target_properties(org_common_net_receiver PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    OUTPUT_NAME "org_common_net_receiver"
    DEBUG_POSTFIX "d"
)

target_include_directories(org_common_net_receiver PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CTK_INCLUDE_DIR}/ctk-0.1
)

target_link_libraries(org_common_net_receiver PRIVATE
    plugin_api
    ${OSGI_QT_CORE_TARGET}
    ${OSGI_QT_NETWORK_TARGET}
    "${NET_RECEIVER_CTK_PLUGINFW_LIB}"
    "${NET_RECEIVER_CTK_CORE_LIB}"
)

if(OSGI_QT_CORE5COMPAT_TARGET)
    target_link_libraries(org_common_net_receiver PRIVATE ${OSGI_QT_CORE5COMPAT_TARGET})
endif()
