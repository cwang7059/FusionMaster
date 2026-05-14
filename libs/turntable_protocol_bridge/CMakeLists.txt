add_library(turntable_protocol_bridge STATIC
    src/turntable_protocol_bridge.cpp
    include/turntable_protocol_bridge/turntable_protocol_bridge.h
)

set_target_properties(turntable_protocol_bridge PROPERTIES
    AUTOMOC ON
)

target_include_directories(turntable_protocol_bridge
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(turntable_protocol_bridge
    PUBLIC
        plugin_api
    PRIVATE
        ${OSGI_QT_CORE_TARGET}
        ${OSGI_QT_NETWORK_TARGET}
        ${OSGI_QT_SERIALPORT_TARGET}
)
