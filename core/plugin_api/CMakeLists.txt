add_library(plugin_api INTERFACE)

target_include_directories(plugin_api INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(plugin_api INTERFACE
    ${OSGI_QT_CORE_TARGET}
)
