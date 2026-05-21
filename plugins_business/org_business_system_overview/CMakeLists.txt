add_library(org_business_system_overview SHARED
    src/system_overview_plugin.cpp
    src/system_overview_plugin.h
    src/system_overview_activator.cpp
    src/system_overview_activator.h
    resources/system_overview_manifest.qrc
)
set_property(GLOBAL APPEND PROPERTY OSGI_RUNTIME_PLUGIN_TARGETS org_business_system_overview)

if(NOT OSGI_CTK_AVAILABLE)
    message(FATAL_ERROR "org_business_system_overview requires CTK runtime support")
endif()

file(GLOB SYSTEM_OVERVIEW_CTK_PLUGINFW_LIB_CANDIDATES
    "${CTK_LIB_DIR}/*CTKPluginFramework*.lib"
    "${CTK_LIB_DIR}/*CTKPluginFramework*.a"
    "${CTK_LIB_DIR}/*CTKPluginFramework*.so"
    "${CTK_LIB_DIR}/*CTKPluginFramework*.so.*"
)

file(GLOB SYSTEM_OVERVIEW_CTK_CORE_LIB_CANDIDATES
    "${CTK_LIB_DIR}/*CTKCore*.lib"
    "${CTK_LIB_DIR}/*CTKCore*.a"
    "${CTK_LIB_DIR}/*CTKCore*.so"
    "${CTK_LIB_DIR}/*CTKCore*.so.*"
)

list(LENGTH SYSTEM_OVERVIEW_CTK_PLUGINFW_LIB_CANDIDATES SYSTEM_OVERVIEW_CTK_PLUGINFW_LIB_COUNT)
list(LENGTH SYSTEM_OVERVIEW_CTK_CORE_LIB_CANDIDATES SYSTEM_OVERVIEW_CTK_CORE_LIB_COUNT)

if(SYSTEM_OVERVIEW_CTK_PLUGINFW_LIB_COUNT EQUAL 0 OR SYSTEM_OVERVIEW_CTK_CORE_LIB_COUNT EQUAL 0)
    message(FATAL_ERROR "CTKPluginFramework/CTKCore libraries were not found in ${CTK_LIB_DIR}")
endif()

list(GET SYSTEM_OVERVIEW_CTK_PLUGINFW_LIB_CANDIDATES 0 SYSTEM_OVERVIEW_CTK_PLUGINFW_LIB)
list(GET SYSTEM_OVERVIEW_CTK_CORE_LIB_CANDIDATES 0 SYSTEM_OVERVIEW_CTK_CORE_LIB)

set_target_properties(org_business_system_overview PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    OUTPUT_NAME "org_business_system_overview"
    DEBUG_POSTFIX "d"
)

target_include_directories(org_business_system_overview PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CTK_INCLUDE_DIR}/ctk-0.1
)

target_link_libraries(org_business_system_overview PRIVATE
    plugin_api
    ${OSGI_QT_CORE_TARGET}
    "${SYSTEM_OVERVIEW_CTK_PLUGINFW_LIB}"
    "${SYSTEM_OVERVIEW_CTK_CORE_LIB}"
)

if(OSGI_QT_CORE5COMPAT_TARGET)
    target_link_libraries(org_business_system_overview PRIVATE ${OSGI_QT_CORE5COMPAT_TARGET})
endif()

osgi_attach_qml_sources_to_current_target(org_business_system_overview)
