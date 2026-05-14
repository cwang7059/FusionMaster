add_library(protocol_55aa_device_bridge STATIC
    src/protocol_55aa_device_bridge.cpp
    include/protocol_55aa_device_bridge/encoder_55aa_device_bridge.h
    include/protocol_55aa_device_bridge/eddy_current_55aa_device_bridge.h
)

set_target_properties(protocol_55aa_device_bridge PROPERTIES
    AUTOMOC ON
)

target_include_directories(protocol_55aa_device_bridge
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(protocol_55aa_device_bridge
    PUBLIC
        plugin_api
    PRIVATE
        ${OSGI_QT_CORE_TARGET}
        ${OSGI_QT_SERIALPORT_TARGET}
)
