add_library(org_common_record_replay SHARED
    src/record_replay_plugin.cpp
    src/record_replay_plugin.h
    src/record_replay_controller.cpp
    src/record_replay_controller.h
    src/replay_runtime_state_service.cpp
    src/replay_runtime_state_service.h
    src/record_replay_activator.cpp
    src/record_replay_activator.h
    resources/record_replay_manifest.qrc
)
set_property(GLOBAL APPEND PROPERTY OSGI_RUNTIME_PLUGIN_TARGETS org_common_record_replay)

if(NOT OSGI_CTK_AVAILABLE)
    message(FATAL_ERROR "org_common_record_replay 需要 CTK 运行时支持，请先准备 third_party/ctk")
endif()

file(GLOB RECORD_REPLAY_CTK_PLUGINFW_LIB_CANDIDATES
    "${CTK_LIB_DIR}/*CTKPluginFramework*.lib"
    "${CTK_LIB_DIR}/*CTKPluginFramework*.a"
    "${CTK_LIB_DIR}/*CTKPluginFramework*.so"
    "${CTK_LIB_DIR}/*CTKPluginFramework*.so.*"
)

file(GLOB RECORD_REPLAY_CTK_CORE_LIB_CANDIDATES
    "${CTK_LIB_DIR}/*CTKCore*.lib"
    "${CTK_LIB_DIR}/*CTKCore*.a"
    "${CTK_LIB_DIR}/*CTKCore*.so"
    "${CTK_LIB_DIR}/*CTKCore*.so.*"
)

list(LENGTH RECORD_REPLAY_CTK_PLUGINFW_LIB_CANDIDATES RECORD_REPLAY_CTK_PLUGINFW_LIB_COUNT)
list(LENGTH RECORD_REPLAY_CTK_CORE_LIB_CANDIDATES RECORD_REPLAY_CTK_CORE_LIB_COUNT)

if(RECORD_REPLAY_CTK_PLUGINFW_LIB_COUNT EQUAL 0 OR RECORD_REPLAY_CTK_CORE_LIB_COUNT EQUAL 0)
    message(FATAL_ERROR "未在 ${CTK_LIB_DIR} 找到 CTKPluginFramework/CTKCore 库文件")
endif()

list(GET RECORD_REPLAY_CTK_PLUGINFW_LIB_CANDIDATES 0 RECORD_REPLAY_CTK_PLUGINFW_LIB)
list(GET RECORD_REPLAY_CTK_CORE_LIB_CANDIDATES 0 RECORD_REPLAY_CTK_CORE_LIB)

set_target_properties(org_common_record_replay PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    OUTPUT_NAME "org_common_record_replay"
    DEBUG_POSTFIX "d"
)

target_include_directories(org_common_record_replay PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CTK_INCLUDE_DIR}/ctk-0.1
)

target_link_libraries(org_common_record_replay PRIVATE
    plugin_api
    ${OSGI_QT_CORE_TARGET}
    ${OSGI_QT_WIDGETS_TARGET}
    "${RECORD_REPLAY_CTK_PLUGINFW_LIB}"
    "${RECORD_REPLAY_CTK_CORE_LIB}"
)

if(OSGI_QT_CORE5COMPAT_TARGET)
    target_link_libraries(org_common_record_replay PRIVATE ${OSGI_QT_CORE5COMPAT_TARGET})
endif()

osgi_attach_qml_sources_to_current_target(org_common_record_replay)
