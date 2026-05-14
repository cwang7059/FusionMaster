add_library(org_common_device_registry SHARED
    src/device_registry_plugin.cpp
    src/device_registry_plugin.h
    src/device_registry_activator.cpp
    src/device_registry_activator.h
    src/device_registry_service.cpp
    src/device_registry_service.h
    resources/device_registry_manifest.qrc
)
set_property(GLOBAL APPEND PROPERTY OSGI_RUNTIME_PLUGIN_TARGETS org_common_device_registry)

if(NOT OSGI_CTK_AVAILABLE)
    message(FATAL_ERROR "org_common_device_registry 闇€瑕?CTK 杩愯鏃舵敮鎸侊紝璇峰厛鍑嗗 third_party/ctk")
endif()

file(GLOB DEVICE_REGISTRY_CTK_PLUGINFW_LIB_CANDIDATES
    "${CTK_LIB_DIR}/*CTKPluginFramework*.lib"
    "${CTK_LIB_DIR}/*CTKPluginFramework*.a"
    "${CTK_LIB_DIR}/*CTKPluginFramework*.so"
    "${CTK_LIB_DIR}/*CTKPluginFramework*.so.*"
)

file(GLOB DEVICE_REGISTRY_CTK_CORE_LIB_CANDIDATES
    "${CTK_LIB_DIR}/*CTKCore*.lib"
    "${CTK_LIB_DIR}/*CTKCore*.a"
    "${CTK_LIB_DIR}/*CTKCore*.so"
    "${CTK_LIB_DIR}/*CTKCore*.so.*"
)

list(LENGTH DEVICE_REGISTRY_CTK_PLUGINFW_LIB_CANDIDATES DEVICE_REGISTRY_CTK_PLUGINFW_LIB_COUNT)
list(LENGTH DEVICE_REGISTRY_CTK_CORE_LIB_CANDIDATES DEVICE_REGISTRY_CTK_CORE_LIB_COUNT)

if(DEVICE_REGISTRY_CTK_PLUGINFW_LIB_COUNT EQUAL 0 OR DEVICE_REGISTRY_CTK_CORE_LIB_COUNT EQUAL 0)
    message(FATAL_ERROR "未在 ${CTK_LIB_DIR} 找到 CTKPluginFramework/CTKCore 库文件")
endif()

list(GET DEVICE_REGISTRY_CTK_PLUGINFW_LIB_CANDIDATES 0 DEVICE_REGISTRY_CTK_PLUGINFW_LIB)
list(GET DEVICE_REGISTRY_CTK_CORE_LIB_CANDIDATES 0 DEVICE_REGISTRY_CTK_CORE_LIB)

set_target_properties(org_common_device_registry PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    OUTPUT_NAME "org_common_device_registry"
    DEBUG_POSTFIX "d"
)

target_include_directories(org_common_device_registry PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CTK_INCLUDE_DIR}/ctk-0.1
)

target_link_libraries(org_common_device_registry PRIVATE
    plugin_api
    ${OSGI_QT_CORE_TARGET}
    "${DEVICE_REGISTRY_CTK_PLUGINFW_LIB}"
    "${DEVICE_REGISTRY_CTK_CORE_LIB}"
)

if(OSGI_QT_CORE5COMPAT_TARGET)
    target_link_libraries(org_common_device_registry PRIVATE ${OSGI_QT_CORE5COMPAT_TARGET})
endif()
