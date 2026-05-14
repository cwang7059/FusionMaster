cmake_minimum_required(VERSION 3.16)

add_executable(field_simulator
    main.cpp
    field_simulator_app.cpp
    field_simulator_app.h
)

target_link_libraries(field_simulator PRIVATE
    ${OSGI_QT_CORE_TARGET}
    ${OSGI_QT_NETWORK_TARGET}
)

set_target_properties(field_simulator PROPERTIES
    AUTOMOC ON
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tools
)

if(MSVC)
    target_compile_options(field_simulator PRIVATE /utf-8)
endif()

target_compile_definitions(field_simulator PRIVATE
    OSGI_FFMPEG_BIN_DIR="$<IF:$<BOOL:${FFMPEG_FOUND}>,${FFMPEG_BIN_DIR},>"
)

if(BUILD_TESTING)
    add_test(
        NAME field_simulator_dry_run
        COMMAND field_simulator --dry-run
    )
endif()
