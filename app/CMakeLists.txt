set(OSGI_APP_SOURCES
    main.cpp
    resources/app_qml_resources.qrc
)

if(WIN32)
    list(APPEND OSGI_APP_SOURCES app.rc)
endif()

# 为 Visual Studio 解决方案补充 QML/JS 展示项，便于直接在 .sln 中编辑界面文件。
file(GLOB_RECURSE OSGI_APP_QML_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/qml/*.qml"
    "${CMAKE_CURRENT_SOURCE_DIR}/qml/*.js"
    "${CMAKE_CURRENT_SOURCE_DIR}/qml/*.mjs"
    "${CMAKE_CURRENT_SOURCE_DIR}/qml/*.json"
    "${CMAKE_CURRENT_SOURCE_DIR}/qml/qmldir"
)
if(OSGI_APP_QML_FILES)
    set_source_files_properties(${OSGI_APP_QML_FILES} PROPERTIES HEADER_FILE_ONLY ON)
    source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/qml" PREFIX "QML Files" FILES ${OSGI_APP_QML_FILES})
    list(APPEND OSGI_APP_SOURCES ${OSGI_APP_QML_FILES})
endif()

add_executable(app ${OSGI_APP_SOURCES})

target_link_libraries(app PRIVATE
    plugin_manager
    message_bus
    logging
    window_manager
    ${OSGI_QT_CORE_TARGET}
    ${OSGI_QT_GUI_TARGET}
    ${OSGI_QT_WIDGETS_TARGET}
    ${OSGI_QT_QML_TARGET}
    ${OSGI_QT_QUICK_TARGET}
    ${OSGI_QT_QUICKCONTROLS2_TARGET}
    ${OSGI_QT_SQL_TARGET}
)

get_property(OSGI_RUNTIME_PLUGIN_TARGETS GLOBAL PROPERTY OSGI_RUNTIME_PLUGIN_TARGETS)
if(OSGI_RUNTIME_PLUGIN_TARGETS)
    list(REMOVE_DUPLICATES OSGI_RUNTIME_PLUGIN_TARGETS)
    add_dependencies(app ${OSGI_RUNTIME_PLUGIN_TARGETS})
endif()

set_target_properties(app PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

if(WIN32)
    set(OSGI_QT_PACKAGE_DIR "")
    if(OSGI_QT_PACKAGE STREQUAL "Qt5" AND DEFINED Qt5_DIR)
        set(OSGI_QT_PACKAGE_DIR "${Qt5_DIR}")
    elseif(OSGI_QT_PACKAGE STREQUAL "Qt6" AND DEFINED Qt6_DIR)
        set(OSGI_QT_PACKAGE_DIR "${Qt6_DIR}")
    endif()

    if(OSGI_QT_PACKAGE_DIR)
        get_filename_component(OSGI_QT_CMAKE_DIR "${OSGI_QT_PACKAGE_DIR}" DIRECTORY)
        get_filename_component(OSGI_QT_LIB_DIR "${OSGI_QT_CMAKE_DIR}" DIRECTORY)
        get_filename_component(OSGI_QT_RUNTIME_ROOT "${OSGI_QT_LIB_DIR}" DIRECTORY)
        set(OSGI_WINDEPLOYQT_EXECUTABLE "${OSGI_QT_RUNTIME_ROOT}/bin/windeployqt.exe")
        set(OSGI_QT_SQLDRIVER_DIR "${OSGI_QT_RUNTIME_ROOT}/plugins/sqldrivers")
    endif()

    if(EXISTS "${OSGI_WINDEPLOYQT_EXECUTABLE}")
        if(DEFINED ENV{SystemRoot} AND NOT "$ENV{SystemRoot}" STREQUAL "")
            file(TO_CMAKE_PATH "$ENV{SystemRoot}" OSGI_SYSTEM_ROOT_DIR)
        else()
            set(OSGI_SYSTEM_ROOT_DIR "C:/Windows")
        endif()

        set(OSGI_WINDEPLOY_ENV_PATH
            "${OSGI_QT_RUNTIME_ROOT}/bin$<SEMICOLON>${OSGI_SYSTEM_ROOT_DIR}/System32$<SEMICOLON>${OSGI_SYSTEM_ROOT_DIR}$<SEMICOLON>${OSGI_SYSTEM_ROOT_DIR}/System32/Wbem"
        )

        add_custom_command(TARGET app POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E env
                "PATH=${OSGI_WINDEPLOY_ENV_PATH}"
                "QTDIR=${OSGI_QT_RUNTIME_ROOT}"
                "QT_PLUGIN_PATH="
                "QML2_IMPORT_PATH="
                "${OSGI_WINDEPLOYQT_EXECUTABLE}"
                --compiler-runtime
                --qmldir "${CMAKE_SOURCE_DIR}/app/qml"
                "$<TARGET_FILE:app>"
            WORKING_DIRECTORY "$<TARGET_FILE_DIR:app>"
            VERBATIM
        )
    else()
        message(WARNING "app: windeployqt.exe not found, Qt runtime deployment may be incomplete")
    endif()

    if(EXISTS "${OSGI_QT_SQLDRIVER_DIR}/qsqlite.dll" OR EXISTS "${OSGI_QT_SQLDRIVER_DIR}/qsqlited.dll")
        add_custom_command(TARGET app POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:app>/sqldrivers"
            VERBATIM
        )

        if(EXISTS "${OSGI_QT_SQLDRIVER_DIR}/qsqlite.dll")
            add_custom_command(TARGET app POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${OSGI_QT_SQLDRIVER_DIR}/qsqlite.dll"
                    "$<TARGET_FILE_DIR:app>/sqldrivers/qsqlite.dll"
                VERBATIM
            )
        endif()

        if(EXISTS "${OSGI_QT_SQLDRIVER_DIR}/qsqlited.dll")
            add_custom_command(TARGET app POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${OSGI_QT_SQLDRIVER_DIR}/qsqlited.dll"
                    "$<TARGET_FILE_DIR:app>/sqldrivers/qsqlited.dll"
                VERBATIM
            )
        endif()
    endif()
endif()

set(UI_THEME_SOURCE_DIR "${CMAKE_SOURCE_DIR}/core/ui_theme")
set(UI_THEME_RUNTIME_DIR "$<TARGET_FILE_DIR:app>/UITheme")

add_custom_command(TARGET app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${UI_THEME_RUNTIME_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${UI_THEME_SOURCE_DIR}/qmldir"
        "${UI_THEME_RUNTIME_DIR}/qmldir"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${UI_THEME_SOURCE_DIR}/qml"
        "${UI_THEME_RUNTIME_DIR}"
    VERBATIM
)

if(OSGI_CTK_AVAILABLE AND WIN32)
    set(OSGI_CTK_CORE_DLL
        "$<IF:$<CONFIG:Debug>,${CTK_BIN_DIR_DEBUG}/CTKCore.dll,${CTK_BIN_DIR_RELEASE}/CTKCore.dll>")
    set(OSGI_CTK_PLUGINFRAMEWORK_DLL
        "$<IF:$<CONFIG:Debug>,${CTK_BIN_DIR_DEBUG}/CTKPluginFramework.dll,${CTK_BIN_DIR_RELEASE}/CTKPluginFramework.dll>")
    add_custom_command(TARGET app POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${OSGI_CTK_CORE_DLL}"
            "$<TARGET_FILE_DIR:app>/CTKCore.dll"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${OSGI_CTK_PLUGINFRAMEWORK_DLL}"
            "$<TARGET_FILE_DIR:app>/CTKPluginFramework.dll"
        VERBATIM
    )
endif()

if(WIN32 AND ZEROMQ_FOUND)
    file(GLOB OSGI_ZEROMQ_RUNTIME_DLLS "${ZEROMQ_BIN_DIR}/*.dll")
    foreach(zmq_dll ${OSGI_ZEROMQ_RUNTIME_DLLS})
        add_custom_command(TARGET app POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${zmq_dll}"
                "$<TARGET_FILE_DIR:app>/"
            VERBATIM
        )
    endforeach()
endif()

get_target_property(OSGI_PROTO_MESSAGES_TARGET_TYPE proto_messages TYPE)
if(WIN32 AND OSGI_PROTO_MESSAGES_TARGET_TYPE STREQUAL "SHARED_LIBRARY")
    add_custom_command(TARGET app POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:proto_messages>"
            "$<TARGET_FILE_DIR:app>/"
        VERBATIM
    )
endif()

if(WIN32 AND PROTOBUF_FOUND)
    file(GLOB OSGI_PROTOBUF_RUNTIME_DLLS "${PROTOBUF_BIN_DIR}/*.dll")
    foreach(pb_dll ${OSGI_PROTOBUF_RUNTIME_DLLS})
        add_custom_command(TARGET app POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${pb_dll}"
                "$<TARGET_FILE_DIR:app>/"
            VERBATIM
        )
    endforeach()
endif()

if(WIN32 AND OSGI_QT_SERIALPORT_TARGET)
    add_custom_command(TARGET app POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:${OSGI_QT_SERIALPORT_TARGET}>"
            "$<TARGET_FILE_DIR:app>/"
        VERBATIM
    )
endif()

if(WIN32 AND OSGI_QT_SQL_TARGET)
    add_custom_command(TARGET app POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:${OSGI_QT_SQL_TARGET}>"
            "$<TARGET_FILE_DIR:app>/"
        VERBATIM
    )
endif()

if(MSVC)
    file(TO_NATIVE_PATH "${CMAKE_SOURCE_DIR}/profiles/default.json" OSGI_DEFAULT_PROFILE_PATH)
    set_target_properties(app PROPERTIES
        VS_DEBUGGER_COMMAND_ARGUMENTS "--profile \"${OSGI_DEFAULT_PROFILE_PATH}\""
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )
endif()
