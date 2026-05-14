if(NOT OSGI_CTK_AVAILABLE)
    message(FATAL_ERROR "org_common_rtsp_replay 需要 CTK 运行时支持，请先准备 third_party/ctk")
endif()

if(NOT OSGI_FFMPEG_AVAILABLE)
    message(WARNING "org_common_rtsp_replay: 未检测到 FFmpeg 产物，跳过构建")
    return()
endif()

file(GLOB RTSP_REPLAY_CTK_PLUGINFW_LIB_CANDIDATES
    "${CTK_LIB_DIR}/*CTKPluginFramework*.lib"
    "${CTK_LIB_DIR}/*CTKPluginFramework*.a"
    "${CTK_LIB_DIR}/*CTKPluginFramework*.so"
    "${CTK_LIB_DIR}/*CTKPluginFramework*.so.*"
)

file(GLOB RTSP_REPLAY_CTK_CORE_LIB_CANDIDATES
    "${CTK_LIB_DIR}/*CTKCore*.lib"
    "${CTK_LIB_DIR}/*CTKCore*.a"
    "${CTK_LIB_DIR}/*CTKCore*.so"
    "${CTK_LIB_DIR}/*CTKCore*.so.*"
)

list(LENGTH RTSP_REPLAY_CTK_PLUGINFW_LIB_CANDIDATES RTSP_REPLAY_CTK_PLUGINFW_LIB_COUNT)
list(LENGTH RTSP_REPLAY_CTK_CORE_LIB_CANDIDATES RTSP_REPLAY_CTK_CORE_LIB_COUNT)

if(RTSP_REPLAY_CTK_PLUGINFW_LIB_COUNT EQUAL 0 OR RTSP_REPLAY_CTK_CORE_LIB_COUNT EQUAL 0)
    message(FATAL_ERROR "未在 ${CTK_LIB_DIR} 找到 CTKPluginFramework/CTKCore 库文件")
endif()

list(GET RTSP_REPLAY_CTK_PLUGINFW_LIB_CANDIDATES 0 RTSP_REPLAY_CTK_PLUGINFW_LIB)
list(GET RTSP_REPLAY_CTK_CORE_LIB_CANDIDATES 0 RTSP_REPLAY_CTK_CORE_LIB)

find_library(RTSP_REPLAY_AVFORMAT_LIB NAMES avformat PATHS "${FFMPEG_LIB_DIR}" NO_DEFAULT_PATH)
find_library(RTSP_REPLAY_AVCODEC_LIB NAMES avcodec PATHS "${FFMPEG_LIB_DIR}" NO_DEFAULT_PATH)
find_library(RTSP_REPLAY_AVUTIL_LIB NAMES avutil PATHS "${FFMPEG_LIB_DIR}" NO_DEFAULT_PATH)
find_library(RTSP_REPLAY_SWSCALE_LIB NAMES swscale PATHS "${FFMPEG_LIB_DIR}" NO_DEFAULT_PATH)
find_library(RTSP_REPLAY_SWRESAMPLE_LIB NAMES swresample PATHS "${FFMPEG_LIB_DIR}" NO_DEFAULT_PATH)

if(NOT RTSP_REPLAY_AVFORMAT_LIB
   OR NOT RTSP_REPLAY_AVCODEC_LIB
   OR NOT RTSP_REPLAY_AVUTIL_LIB
   OR NOT RTSP_REPLAY_SWSCALE_LIB
   OR NOT RTSP_REPLAY_SWRESAMPLE_LIB)
    message(FATAL_ERROR "org_common_rtsp_replay: 未在 ${FFMPEG_LIB_DIR} 找到完整 FFmpeg 库（至少 avformat/avcodec/avutil/swscale/swresample）")
endif()

add_library(org_common_rtsp_replay SHARED
    src/rtsp_replay_plugin.cpp
    src/rtsp_replay_plugin.h
    src/rtsp_replay_activator.cpp
    src/rtsp_replay_activator.h
    resources/rtsp_replay_manifest.qrc
)
set_property(GLOBAL APPEND PROPERTY OSGI_RUNTIME_PLUGIN_TARGETS org_common_rtsp_replay)

set_target_properties(org_common_rtsp_replay PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    OUTPUT_NAME "org_common_rtsp_replay"
    DEBUG_POSTFIX "d"
)

target_include_directories(org_common_rtsp_replay PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CTK_INCLUDE_DIR}/ctk-0.1
    ${FFMPEG_INCLUDE_DIR}
)

target_link_libraries(org_common_rtsp_replay PRIVATE
    plugin_api
    ${OSGI_QT_CORE_TARGET}
    "${RTSP_REPLAY_CTK_PLUGINFW_LIB}"
    "${RTSP_REPLAY_CTK_CORE_LIB}"
    "${RTSP_REPLAY_AVFORMAT_LIB}"
    "${RTSP_REPLAY_AVCODEC_LIB}"
    "${RTSP_REPLAY_AVUTIL_LIB}"
    "${RTSP_REPLAY_SWSCALE_LIB}"
    "${RTSP_REPLAY_SWRESAMPLE_LIB}"
)

if(OSGI_QT_CORE5COMPAT_TARGET)
    target_link_libraries(org_common_rtsp_replay PRIVATE ${OSGI_QT_CORE5COMPAT_TARGET})
endif()

if(WIN32 AND FFMPEG_FOUND)
    file(GLOB RTSP_REPLAY_FFMPEG_DLLS "${FFMPEG_BIN_DIR}/*.dll")
    foreach(ffmpeg_dll ${RTSP_REPLAY_FFMPEG_DLLS})
        add_custom_command(TARGET org_common_rtsp_replay POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/bin/Release"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/bin/Debug"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${ffmpeg_dll}"
                "$<TARGET_FILE_DIR:org_common_rtsp_replay>/"
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
