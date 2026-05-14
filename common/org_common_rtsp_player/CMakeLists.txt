if(NOT OSGI_CTK_AVAILABLE)
    message(FATAL_ERROR "org_common_rtsp_player 需要 CTK 运行时支持，请先准备 third_party/ctk")
endif()

if(NOT OSGI_FFMPEG_AVAILABLE)
    message(WARNING "org_common_rtsp_player: 未检测到 FFmpeg 产物，跳过构建")
    return()
endif()

file(GLOB RTSP_PLAYER_CTK_PLUGINFW_LIB_CANDIDATES
    "${CTK_LIB_DIR}/*CTKPluginFramework*.lib"
    "${CTK_LIB_DIR}/*CTKPluginFramework*.a"
    "${CTK_LIB_DIR}/*CTKPluginFramework*.so"
    "${CTK_LIB_DIR}/*CTKPluginFramework*.so.*"
)

file(GLOB RTSP_PLAYER_CTK_CORE_LIB_CANDIDATES
    "${CTK_LIB_DIR}/*CTKCore*.lib"
    "${CTK_LIB_DIR}/*CTKCore*.a"
    "${CTK_LIB_DIR}/*CTKCore*.so"
    "${CTK_LIB_DIR}/*CTKCore*.so.*"
)

list(LENGTH RTSP_PLAYER_CTK_PLUGINFW_LIB_CANDIDATES RTSP_PLAYER_CTK_PLUGINFW_LIB_COUNT)
list(LENGTH RTSP_PLAYER_CTK_CORE_LIB_CANDIDATES RTSP_PLAYER_CTK_CORE_LIB_COUNT)

if(RTSP_PLAYER_CTK_PLUGINFW_LIB_COUNT EQUAL 0 OR RTSP_PLAYER_CTK_CORE_LIB_COUNT EQUAL 0)
    message(FATAL_ERROR "未在 ${CTK_LIB_DIR} 找到 CTKPluginFramework/CTKCore 库文件")
endif()

list(GET RTSP_PLAYER_CTK_PLUGINFW_LIB_CANDIDATES 0 RTSP_PLAYER_CTK_PLUGINFW_LIB)
list(GET RTSP_PLAYER_CTK_CORE_LIB_CANDIDATES 0 RTSP_PLAYER_CTK_CORE_LIB)

find_library(RTSP_PLAYER_AVCODEC_LIB NAMES avcodec PATHS "${FFMPEG_LIB_DIR}" NO_DEFAULT_PATH)
find_library(RTSP_PLAYER_AVUTIL_LIB NAMES avutil PATHS "${FFMPEG_LIB_DIR}" NO_DEFAULT_PATH)
find_library(RTSP_PLAYER_SWSCALE_LIB NAMES swscale PATHS "${FFMPEG_LIB_DIR}" NO_DEFAULT_PATH)

if(NOT RTSP_PLAYER_AVCODEC_LIB
   OR NOT RTSP_PLAYER_AVUTIL_LIB
   OR NOT RTSP_PLAYER_SWSCALE_LIB)
    message(FATAL_ERROR "org_common_rtsp_player: 未在 ${FFMPEG_LIB_DIR} 找到 avcodec/avutil/swscale")
endif()

add_library(org_common_rtsp_player SHARED
    src/rtsp_player_plugin.cpp
    src/rtsp_player_plugin.h
    src/rtsp_player_controller.cpp
    src/rtsp_player_controller.h
    src/rtsp_player_activator.cpp
    src/rtsp_player_activator.h
    src/rtsp_video_item.cpp
    src/rtsp_video_item.h
    resources/rtsp_player_manifest.qrc
)
set_property(GLOBAL APPEND PROPERTY OSGI_RUNTIME_PLUGIN_TARGETS org_common_rtsp_player)

set_target_properties(org_common_rtsp_player PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    OUTPUT_NAME "org_common_rtsp_player"
    DEBUG_POSTFIX "d"
)

target_include_directories(org_common_rtsp_player PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CTK_INCLUDE_DIR}/ctk-0.1
    ${FFMPEG_INCLUDE_DIR}
)

target_link_libraries(org_common_rtsp_player PRIVATE
    plugin_api
    ${OSGI_QT_CORE_TARGET}
    ${OSGI_QT_GUI_TARGET}
    ${OSGI_QT_QML_TARGET}
    ${OSGI_QT_QUICK_TARGET}
    "${RTSP_PLAYER_CTK_PLUGINFW_LIB}"
    "${RTSP_PLAYER_CTK_CORE_LIB}"
    "${RTSP_PLAYER_AVCODEC_LIB}"
    "${RTSP_PLAYER_AVUTIL_LIB}"
    "${RTSP_PLAYER_SWSCALE_LIB}"
)

if(OSGI_QT_CORE5COMPAT_TARGET)
    target_link_libraries(org_common_rtsp_player PRIVATE ${OSGI_QT_CORE5COMPAT_TARGET})
endif()

if(WIN32 AND FFMPEG_FOUND)
    file(GLOB RTSP_PLAYER_FFMPEG_DLLS "${FFMPEG_BIN_DIR}/*.dll")
    foreach(ffmpeg_dll ${RTSP_PLAYER_FFMPEG_DLLS})
        add_custom_command(TARGET org_common_rtsp_player POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/bin/Release"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/bin/Debug"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${ffmpeg_dll}"
                "$<TARGET_FILE_DIR:org_common_rtsp_player>/"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${ffmpeg_dll}"
                "${CMAKE_BINARY_DIR}/bin/Release/"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${ffmpeg_dll}"
                "${CMAKE_BINARY_DIR}/bin/Debug/"
            VERBATIM
        )
    endforeach()
endif()

osgi_attach_qml_sources_to_current_target(org_common_rtsp_player)
