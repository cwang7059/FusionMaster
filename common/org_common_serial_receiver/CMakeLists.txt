add_library(org_common_serial_receiver SHARED
    src/serial_receiver_plugin.cpp
    src/serial_receiver_plugin.h
    src/serial_receiver_activator.cpp
    src/serial_receiver_activator.h
    resources/serial_receiver_manifest.qrc
)
set_property(GLOBAL APPEND PROPERTY OSGI_RUNTIME_PLUGIN_TARGETS org_common_serial_receiver)

if(NOT OSGI_CTK_AVAILABLE)
    message(FATAL_ERROR "org_common_serial_receiver 需要 CTK 运行时支持，请先准备 third_party/ctk")
endif()

file(GLOB SERIAL_RECEIVER_CTK_PLUGINFW_LIB_CANDIDATES
    "${CTK_LIB_DIR}/*CTKPluginFramework*.lib"
    "${CTK_LIB_DIR}/*CTKPluginFramework*.a"
    "${CTK_LIB_DIR}/*CTKPluginFramework*.so"
    "${CTK_LIB_DIR}/*CTKPluginFramework*.so.*"
)

file(GLOB SERIAL_RECEIVER_CTK_CORE_LIB_CANDIDATES
    "${CTK_LIB_DIR}/*CTKCore*.lib"
    "${CTK_LIB_DIR}/*CTKCore*.a"
    "${CTK_LIB_DIR}/*CTKCore*.so"
    "${CTK_LIB_DIR}/*CTKCore*.so.*"
)

list(LENGTH SERIAL_RECEIVER_CTK_PLUGINFW_LIB_CANDIDATES SERIAL_RECEIVER_CTK_PLUGINFW_LIB_COUNT)
list(LENGTH SERIAL_RECEIVER_CTK_CORE_LIB_CANDIDATES SERIAL_RECEIVER_CTK_CORE_LIB_COUNT)

if(SERIAL_RECEIVER_CTK_PLUGINFW_LIB_COUNT EQUAL 0 OR SERIAL_RECEIVER_CTK_CORE_LIB_COUNT EQUAL 0)
    message(FATAL_ERROR "未在 ${CTK_LIB_DIR} 找到 CTKPluginFramework/CTKCore 库文件")
endif()

list(GET SERIAL_RECEIVER_CTK_PLUGINFW_LIB_CANDIDATES 0 SERIAL_RECEIVER_CTK_PLUGINFW_LIB)
list(GET SERIAL_RECEIVER_CTK_CORE_LIB_CANDIDATES 0 SERIAL_RECEIVER_CTK_CORE_LIB)

set_target_properties(org_common_serial_receiver PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    OUTPUT_NAME "org_common_serial_receiver"
    DEBUG_POSTFIX "d"
)

target_include_directories(org_common_serial_receiver PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CTK_INCLUDE_DIR}/ctk-0.1
)

target_link_libraries(org_common_serial_receiver PRIVATE
    plugin_api
    ${OSGI_QT_CORE_TARGET}
    ${OSGI_QT_SERIALPORT_TARGET}
    "${SERIAL_RECEIVER_CTK_PLUGINFW_LIB}"
    "${SERIAL_RECEIVER_CTK_CORE_LIB}"
)

if(OSGI_QT_CORE5COMPAT_TARGET)
    target_link_libraries(org_common_serial_receiver PRIVATE ${OSGI_QT_CORE5COMPAT_TARGET})
endif()
