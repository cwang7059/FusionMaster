add_library(fast_mirror_protocol_bridge STATIC
    src/fast_mirror_protocol_bridge.cpp
    include/fast_mirror_protocol_bridge/fast_mirror_protocol_bridge.h
)

set_target_properties(fast_mirror_protocol_bridge PROPERTIES
    AUTOMOC ON
)

target_include_directories(fast_mirror_protocol_bridge
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(fast_mirror_protocol_bridge
    PUBLIC
        plugin_api
    PRIVATE
        ${OSGI_QT_CORE_TARGET}
        ${OSGI_QT_SQL_TARGET}
        ${OSGI_QT_SERIALPORT_TARGET}
)
