cmake_minimum_required(VERSION 3.16)
project(osgi_business_dev VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/utf-8)
endif()

set(OSGI_QT_MAJOR "6" CACHE STRING "Qt 主版本，支持 5 或 6")
set_property(CACHE OSGI_QT_MAJOR PROPERTY STRINGS 5 6)

if(OSGI_QT_MAJOR STREQUAL "5")
    set(OSGI_QT_PACKAGE "Qt5")
    set(OSGI_QT_NAMESPACE "Qt5")
    set(OSGI_QT_ROOT "C:/Qt/5.14.2" CACHE PATH "Qt 安装根目录")
    set(_osgi_qt_version_glob "5.*")
    set(_osgi_qt_cmake_subdir "lib/cmake/Qt5")
elseif(OSGI_QT_MAJOR STREQUAL "6")
    set(OSGI_QT_PACKAGE "Qt6")
    set(OSGI_QT_NAMESPACE "Qt6")
    set(OSGI_QT_ROOT "C:/Qt6_10" CACHE PATH "Qt 安装根目录")
    set(_osgi_qt_version_glob "6.*")
    set(_osgi_qt_cmake_subdir "lib/cmake/Qt6")
else()
    message(FATAL_ERROR "OSGI_QT_MAJOR 仅支持 5 或 6，当前值: ${OSGI_QT_MAJOR}")
endif()

set(BUSINESS_PLUGIN_FILTER "*" CACHE STRING "Business plugins to build, '*' or ';' separated names")
set(COMMON_PLUGIN_FILTER "*" CACHE STRING "Common plugins to build, '*' or ';' separated names")
option(ENABLE_DEBUG_ASSISTANT "Enable debug assistant plugin" ON)

if(OSGI_QT_ROOT)
    list(APPEND CMAKE_PREFIX_PATH "${OSGI_QT_ROOT}")

    file(GLOB _osgi_qt_version_dirs LIST_DIRECTORIES TRUE "${OSGI_QT_ROOT}/${_osgi_qt_version_glob}")
    foreach(_version_dir ${_osgi_qt_version_dirs})
        if(IS_DIRECTORY "${_version_dir}")
            list(APPEND CMAKE_PREFIX_PATH "${_version_dir}")

            file(GLOB _osgi_qt_compiler_dirs LIST_DIRECTORIES TRUE "${_version_dir}/*")
            set(_osgi_qt_preferred_compiler_dirs)
            set(_osgi_qt_other_compiler_dirs)
            foreach(_compiler_dir ${_osgi_qt_compiler_dirs})
                if(IS_DIRECTORY "${_compiler_dir}")
                    get_filename_component(_compiler_dir_name "${_compiler_dir}" NAME)
                    if(MSVC AND _compiler_dir_name MATCHES "^msvc")
                        list(APPEND _osgi_qt_preferred_compiler_dirs "${_compiler_dir}")
                    else()
                        list(APPEND _osgi_qt_other_compiler_dirs "${_compiler_dir}")
                    endif()
                endif()
            endforeach()

            foreach(_compiler_dir ${_osgi_qt_preferred_compiler_dirs} ${_osgi_qt_other_compiler_dirs})
                list(APPEND CMAKE_PREFIX_PATH
                    "${_compiler_dir}"
                    "${_compiler_dir}/lib/cmake"
                    "${_compiler_dir}/${_osgi_qt_cmake_subdir}"
                )
            endforeach()
        endif()
    endforeach()
endif()

set(_osgi_qt_components Core Gui Widgets Qml Quick QuickControls2 Network SerialPort Sql Xml Concurrent PrintSupport)
if(OSGI_QT_MAJOR STREQUAL "6")
    list(APPEND _osgi_qt_components Core5Compat)
endif()
find_package(${OSGI_QT_PACKAGE} COMPONENTS ${_osgi_qt_components} REQUIRED)

set(OSGI_QT_CORE_TARGET ${OSGI_QT_NAMESPACE}::Core)
set(OSGI_QT_GUI_TARGET ${OSGI_QT_NAMESPACE}::Gui)
set(OSGI_QT_WIDGETS_TARGET ${OSGI_QT_NAMESPACE}::Widgets)
set(OSGI_QT_QML_TARGET ${OSGI_QT_NAMESPACE}::Qml)
set(OSGI_QT_QUICK_TARGET ${OSGI_QT_NAMESPACE}::Quick)
set(OSGI_QT_QUICKCONTROLS2_TARGET ${OSGI_QT_NAMESPACE}::QuickControls2)
set(OSGI_QT_NETWORK_TARGET ${OSGI_QT_NAMESPACE}::Network)
set(OSGI_QT_SERIALPORT_TARGET ${OSGI_QT_NAMESPACE}::SerialPort)
set(OSGI_QT_SQL_TARGET ${OSGI_QT_NAMESPACE}::Sql)
set(OSGI_QT_XML_TARGET ${OSGI_QT_NAMESPACE}::Xml)
set(OSGI_QT_CONCURRENT_TARGET ${OSGI_QT_NAMESPACE}::Concurrent)
set(OSGI_QT_PRINTSUPPORT_TARGET ${OSGI_QT_NAMESPACE}::PrintSupport)
set(OSGI_QT_CORE5COMPAT_TARGET "")
if(OSGI_QT_MAJOR STREQUAL "6" AND TARGET Qt6::Core5Compat)
    set(OSGI_QT_CORE5COMPAT_TARGET Qt6::Core5Compat)
endif()

set(THIRD_PARTY_DIR "${CMAKE_SOURCE_DIR}/third_party")
if(WIN32)
    set(OSGI_PLATFORM_TAG "win-x64")
    if(MSVC)
        if(CMAKE_GENERATOR MATCHES "Visual Studio 18 2026")
            set(OSGI_MSVC_TAG "vs2026")
        elseif(CMAKE_GENERATOR MATCHES "Visual Studio 17 2022")
            set(OSGI_MSVC_TAG "vs2022")
        elseif(CMAKE_GENERATOR MATCHES "Visual Studio 16 2019")
            set(OSGI_MSVC_TAG "vs2019")
        elseif(MSVC_VERSION GREATER_EQUAL 1930)
            set(OSGI_MSVC_TAG "vs2022")
        elseif(MSVC_VERSION GREATER_EQUAL 1920)
            set(OSGI_MSVC_TAG "vs2019")
        else()
            set(OSGI_MSVC_TAG "vs")
        endif()
    else()
        set(OSGI_MSVC_TAG "toolchain")
    endif()
    set(OSGI_PLATFORM_TAG_TOOLCHAIN "${OSGI_PLATFORM_TAG}-qt${OSGI_QT_MAJOR}-${OSGI_MSVC_TAG}")
elseif(UNIX)
    set(OSGI_PLATFORM_TAG "linux-x64")
    set(OSGI_PLATFORM_TAG_TOOLCHAIN "${OSGI_PLATFORM_TAG}-qt${OSGI_QT_MAJOR}")
else()
    set(OSGI_PLATFORM_TAG "unknown")
    set(OSGI_PLATFORM_TAG_TOOLCHAIN "unknown")
endif()

set(OSGI_THIRD_PARTY_PLATFORM_TAG "" CACHE STRING "第三方库平台目录标签；留空自动推导，例如 win-x64-qt6-vs2026")

function(osgi_pick_platform_tag out_var package_root)
    if(NOT OSGI_THIRD_PARTY_PLATFORM_TAG STREQUAL "")
        if(EXISTS "${package_root}/${OSGI_THIRD_PARTY_PLATFORM_TAG}")
            set(${out_var} "${OSGI_THIRD_PARTY_PLATFORM_TAG}" PARENT_SCOPE)
            return()
        endif()
        message(WARNING "third_party: 指定标签 ${OSGI_THIRD_PARTY_PLATFORM_TAG} 不存在于 ${package_root}，回退自动选择")
    endif()

    if(EXISTS "${package_root}/${OSGI_PLATFORM_TAG_TOOLCHAIN}")
        set(${out_var} "${OSGI_PLATFORM_TAG_TOOLCHAIN}" PARENT_SCOPE)
    else()
        set(${out_var} "${OSGI_PLATFORM_TAG}" PARENT_SCOPE)
    endif()
endfunction()

include(${CMAKE_SOURCE_DIR}/cmake/ThirdParty.cmake)
osgi_pick_platform_tag(CTK_PLATFORM_TAG "${THIRD_PARTY_DIR}/ctk")
osgi_pick_platform_tag(ZEROMQ_PLATFORM_TAG "${THIRD_PARTY_DIR}/zeromq")
osgi_pick_platform_tag(PROTOBUF_PLATFORM_TAG "${THIRD_PARTY_DIR}/protobuf")
osgi_pick_platform_tag(FFMPEG_PLATFORM_TAG "${THIRD_PARTY_DIR}/ffmpeg")

osgi_collect_third_party_status(CTK "${THIRD_PARTY_DIR}/ctk" "${CTK_PLATFORM_TAG}")
osgi_collect_third_party_status(ZEROMQ "${THIRD_PARTY_DIR}/zeromq" "${ZEROMQ_PLATFORM_TAG}")
osgi_collect_third_party_status(PROTOBUF "${THIRD_PARTY_DIR}/protobuf" "${PROTOBUF_PLATFORM_TAG}")
osgi_collect_third_party_status(FFMPEG "${THIRD_PARTY_DIR}/ffmpeg" "${FFMPEG_PLATFORM_TAG}")

# 多配置工程（Debug/Release）下，CTK 调试/发布产物可能位于不同目录。
# 统一推导出两套目录，避免 Release 误拷贝 Debug 的 CTKCore.dll。
set(CTK_PLATFORM_TAG_RELEASE "${CTK_PLATFORM_TAG}")
if(CTK_PLATFORM_TAG_RELEASE MATCHES "-debug$")
    string(REGEX REPLACE "-debug$" "" CTK_PLATFORM_TAG_RELEASE "${CTK_PLATFORM_TAG_RELEASE}")
endif()

set(CTK_PLATFORM_TAG_DEBUG "${CTK_PLATFORM_TAG_RELEASE}-debug")
if(NOT EXISTS "${THIRD_PARTY_DIR}/ctk/${CTK_PLATFORM_TAG_DEBUG}")
    if(CTK_PLATFORM_TAG MATCHES "-debug$")
        set(CTK_PLATFORM_TAG_DEBUG "${CTK_PLATFORM_TAG}")
    else()
        set(CTK_PLATFORM_TAG_DEBUG "${CTK_PLATFORM_TAG_RELEASE}")
    endif()
endif()

set(CTK_LIB_DIR_RELEASE "${THIRD_PARTY_DIR}/ctk/${CTK_PLATFORM_TAG_RELEASE}/lib")
set(CTK_BIN_DIR_RELEASE "${THIRD_PARTY_DIR}/ctk/${CTK_PLATFORM_TAG_RELEASE}/bin")
set(CTK_LIB_DIR_DEBUG "${THIRD_PARTY_DIR}/ctk/${CTK_PLATFORM_TAG_DEBUG}/lib")
set(CTK_BIN_DIR_DEBUG "${THIRD_PARTY_DIR}/ctk/${CTK_PLATFORM_TAG_DEBUG}/bin")

if(CTK_FOUND)
    set(OSGI_CTK_AVAILABLE ON)
else()
    set(OSGI_CTK_AVAILABLE OFF)
endif()

if(FFMPEG_FOUND)
    set(OSGI_FFMPEG_AVAILABLE ON)
else()
    set(OSGI_FFMPEG_AVAILABLE OFF)
endif()

include(${CMAKE_SOURCE_DIR}/cmake/BusinessProto.cmake)

set(LEGACY_PROGRAM_ROOT "${CMAKE_SOURCE_DIR}/../program" CACHE PATH "legacy program root")

add_subdirectory(core/plugin_api)

function(osgi_add_prebuilt_core_target target_name include_dir lib_file)
    set(extra_link_libs ${ARGN})

    if(TARGET ${target_name})
        return()
    endif()

    if(NOT EXISTS "${lib_file}")
        message(FATAL_ERROR "缺少核心库文件: ${lib_file}")
    endif()

    add_library(${target_name} STATIC IMPORTED GLOBAL)

    set(_debug_lib_file "")
    if(MSVC AND lib_file MATCHES "\\.lib$")
        string(REGEX REPLACE "\\.lib$" "d.lib" _candidate_debug_lib "${lib_file}")
        if(EXISTS "${_candidate_debug_lib}")
            set(_debug_lib_file "${_candidate_debug_lib}")
        endif()
    endif()

    if(_debug_lib_file)
        set_target_properties(${target_name} PROPERTIES
            IMPORTED_CONFIGURATIONS "DEBUG;RELEASE;RELWITHDEBINFO;MINSIZEREL"
            IMPORTED_LOCATION_DEBUG "${_debug_lib_file}"
            IMPORTED_LOCATION_RELEASE "${lib_file}"
            IMPORTED_LOCATION_RELWITHDEBINFO "${lib_file}"
            IMPORTED_LOCATION_MINSIZEREL "${lib_file}"
            INTERFACE_INCLUDE_DIRECTORIES "${include_dir}"
        )
    else()
        set_target_properties(${target_name} PROPERTIES
            IMPORTED_LOCATION "${lib_file}"
            INTERFACE_INCLUDE_DIRECTORIES "${include_dir}"
        )
    endif()

    target_link_libraries(${target_name} INTERFACE plugin_api ${extra_link_libs})
endfunction()

if(WIN32)
    file(GLOB OSGI_CTK_PLUGINFW_LIB_RELEASE_CANDIDATES "${CTK_LIB_DIR_RELEASE}/*CTKPluginFramework*.lib")
    file(GLOB OSGI_CTK_CORE_LIB_RELEASE_CANDIDATES "${CTK_LIB_DIR_RELEASE}/*CTKCore*.lib")
    if(NOT OSGI_CTK_PLUGINFW_LIB_RELEASE_CANDIDATES OR NOT OSGI_CTK_CORE_LIB_RELEASE_CANDIDATES)
        message(FATAL_ERROR "未在 ${CTK_LIB_DIR_RELEASE} 找到 Release CTKPluginFramework/CTKCore 库")
    endif()
    list(GET OSGI_CTK_PLUGINFW_LIB_RELEASE_CANDIDATES 0 OSGI_CTK_PLUGINFW_LIB_RELEASE)
    list(GET OSGI_CTK_CORE_LIB_RELEASE_CANDIDATES 0 OSGI_CTK_CORE_LIB_RELEASE)

    file(GLOB OSGI_CTK_PLUGINFW_LIB_DEBUG_CANDIDATES "${CTK_LIB_DIR_DEBUG}/*CTKPluginFramework*.lib")
    file(GLOB OSGI_CTK_CORE_LIB_DEBUG_CANDIDATES "${CTK_LIB_DIR_DEBUG}/*CTKCore*.lib")
    if(OSGI_CTK_PLUGINFW_LIB_DEBUG_CANDIDATES)
        list(GET OSGI_CTK_PLUGINFW_LIB_DEBUG_CANDIDATES 0 OSGI_CTK_PLUGINFW_LIB_DEBUG)
    else()
        set(OSGI_CTK_PLUGINFW_LIB_DEBUG "${OSGI_CTK_PLUGINFW_LIB_RELEASE}")
    endif()
    if(OSGI_CTK_CORE_LIB_DEBUG_CANDIDATES)
        list(GET OSGI_CTK_CORE_LIB_DEBUG_CANDIDATES 0 OSGI_CTK_CORE_LIB_DEBUG)
    else()
        set(OSGI_CTK_CORE_LIB_DEBUG "${OSGI_CTK_CORE_LIB_RELEASE}")
    endif()

    set(OSGI_CTK_PLUGINFW_LIB
        "$<IF:$<CONFIG:Debug>,${OSGI_CTK_PLUGINFW_LIB_DEBUG},${OSGI_CTK_PLUGINFW_LIB_RELEASE}>")
    set(OSGI_CTK_CORE_LIB
        "$<IF:$<CONFIG:Debug>,${OSGI_CTK_CORE_LIB_DEBUG},${OSGI_CTK_CORE_LIB_RELEASE}>")

    file(GLOB OSGI_ZEROMQ_LIB_CANDIDATES "${ZEROMQ_LIB_DIR}/*zmq*.lib")
    if(NOT OSGI_ZEROMQ_LIB_CANDIDATES)
        message(FATAL_ERROR "未在 ${ZEROMQ_LIB_DIR} 找到 ZeroMQ 库")
    endif()
    list(GET OSGI_ZEROMQ_LIB_CANDIDATES 0 OSGI_ZEROMQ_LIB)

    osgi_add_prebuilt_core_target(
        plugin_manager
        "${CMAKE_SOURCE_DIR}/core/plugin_manager/include"
        "${CMAKE_SOURCE_DIR}/core/plugin_manager/win-x64/lib/plugin_manager.lib"
        "${OSGI_CTK_PLUGINFW_LIB}"
        "${OSGI_CTK_CORE_LIB}"
        ${OSGI_QT_CORE5COMPAT_TARGET}
    )
    osgi_add_prebuilt_core_target(
        message_bus
        "${CMAKE_SOURCE_DIR}/core/message_bus/include"
        "${CMAKE_SOURCE_DIR}/core/message_bus/win-x64/lib/message_bus.lib"
        proto_messages
        "${OSGI_ZEROMQ_LIB}"
    )
    osgi_add_prebuilt_core_target(
        logging
        "${CMAKE_SOURCE_DIR}/core/logging/include"
        "${CMAKE_SOURCE_DIR}/core/logging/win-x64/lib/logging.lib"
        ${OSGI_QT_WIDGETS_TARGET}
    )
    osgi_add_prebuilt_core_target(
        window_manager
        "${CMAKE_SOURCE_DIR}/core/window_manager/include"
        "${CMAKE_SOURCE_DIR}/core/window_manager/win-x64/lib/window_manager.lib"
        ${OSGI_QT_GUI_TARGET}
    )
endif()

function(osgi_attach_qml_sources_to_current_target target_name)
    # 仅用于 IDE 展示：在插件自身目录作用域中给 QML 建立分组，便于 VS 编辑。
    if(NOT TARGET ${target_name})
        return()
    endif()

    set(_qml_dir "${CMAKE_CURRENT_SOURCE_DIR}/qml")
    if(NOT EXISTS "${_qml_dir}")
        return()
    endif()

    file(GLOB_RECURSE _qml_files CONFIGURE_DEPENDS
        "${_qml_dir}/*.qml"
        "${_qml_dir}/*.js"
        "${_qml_dir}/*.mjs"
        "${_qml_dir}/*.json"
        "${_qml_dir}/qmldir"
    )
    if(NOT _qml_files)
        return()
    endif()

    set_source_files_properties(${_qml_files} PROPERTIES HEADER_FILE_ONLY ON)
    target_sources(${target_name} PRIVATE ${_qml_files})
    source_group(TREE "${_qml_dir}" PREFIX "QML Files" FILES ${_qml_files})
endfunction()

add_subdirectory(proto)
add_subdirectory(libs)
add_subdirectory(common)
add_subdirectory(plugins_business)

function(osgi_normalize_plugin_output_dirs)
    get_property(_osgi_runtime_plugins GLOBAL PROPERTY OSGI_RUNTIME_PLUGIN_TARGETS)
    if(NOT _osgi_runtime_plugins)
        return()
    endif()

    list(REMOVE_DUPLICATES _osgi_runtime_plugins)
    foreach(_plugin_target ${_osgi_runtime_plugins})
        if(NOT TARGET ${_plugin_target})
            continue()
        endif()

        if(_plugin_target MATCHES "^org_common_")
            set(_plugin_rel_dir "common/${_plugin_target}")
        else()
            set(_plugin_rel_dir "plugins_business/${_plugin_target}")
        endif()

        # 这里不能使用 $<CONFIG>，在自定义目标里会退化为空，导致落到 bin/common。
        # 使用 CMAKE_CFG_INTDIR 可在 VS 多配置下展开为 $(Configuration)。
        set(_plugin_output_dir "${CMAKE_BINARY_DIR}/bin/${CMAKE_CFG_INTDIR}/${_plugin_rel_dir}")
        set(_plugin_output_dir_debug "${CMAKE_BINARY_DIR}/bin/Debug/${_plugin_rel_dir}")
        set(_plugin_output_dir_release "${CMAKE_BINARY_DIR}/bin/Release/${_plugin_rel_dir}")
        set(_plugin_output_dir_relwithdebinfo "${CMAKE_BINARY_DIR}/bin/RelWithDebInfo/${_plugin_rel_dir}")
        set(_plugin_output_dir_minsizerel "${CMAKE_BINARY_DIR}/bin/MinSizeRel/${_plugin_rel_dir}")

        set_target_properties(${_plugin_target} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${_plugin_output_dir}"
            LIBRARY_OUTPUT_DIRECTORY "${_plugin_output_dir}"
            PDB_OUTPUT_DIRECTORY "${_plugin_output_dir}"
            RUNTIME_OUTPUT_DIRECTORY_DEBUG "${_plugin_output_dir_debug}"
            RUNTIME_OUTPUT_DIRECTORY_RELEASE "${_plugin_output_dir_release}"
            RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${_plugin_output_dir_relwithdebinfo}"
            RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${_plugin_output_dir_minsizerel}"
            LIBRARY_OUTPUT_DIRECTORY_DEBUG "${_plugin_output_dir_debug}"
            LIBRARY_OUTPUT_DIRECTORY_RELEASE "${_plugin_output_dir_release}"
            LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${_plugin_output_dir_relwithdebinfo}"
            LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL "${_plugin_output_dir_minsizerel}"
            PDB_OUTPUT_DIRECTORY_DEBUG "${_plugin_output_dir_debug}"
            PDB_OUTPUT_DIRECTORY_RELEASE "${_plugin_output_dir_release}"
            PDB_OUTPUT_DIRECTORY_RELWITHDEBINFO "${_plugin_output_dir_relwithdebinfo}"
            PDB_OUTPUT_DIRECTORY_MINSIZEREL "${_plugin_output_dir_minsizerel}"
            # 保持 Debug 插件文件名与内嵌 qrc 清单前缀一致。
            # CTK 会根据 DLL 基名解析 :/<plugin-symbolic-name>/META-INF/MANIFEST.MF。
            DEBUG_POSTFIX ""
        )
    endforeach()
endfunction()

osgi_normalize_plugin_output_dirs()

function(osgi_sync_plugin_metadata_to_runtime)
    get_property(_osgi_runtime_plugins GLOBAL PROPERTY OSGI_RUNTIME_PLUGIN_TARGETS)
    if(NOT _osgi_runtime_plugins)
        return()
    endif()

    list(REMOVE_DUPLICATES _osgi_runtime_plugins)
    foreach(_plugin_target ${_osgi_runtime_plugins})
        if(NOT TARGET ${_plugin_target})
            continue()
        endif()

        if(_plugin_target MATCHES "^org_common_")
            set(_plugin_source_dir "${CMAKE_SOURCE_DIR}/common/${_plugin_target}")
            set(_plugin_rel_dir "common/${_plugin_target}")
        else()
            set(_plugin_source_dir "${CMAKE_SOURCE_DIR}/plugins_business/${_plugin_target}")
            set(_plugin_rel_dir "plugins_business/${_plugin_target}")
        endif()

        set(_plugin_json "${_plugin_source_dir}/plugin.json")
        if(NOT EXISTS "${_plugin_json}")
            continue()
        endif()
        # 这里使用 CMAKE_CFG_INTDIR，在 VS 多配置下会展开为 $(Configuration)。
        set(_plugin_output_dir "${CMAKE_BINARY_DIR}/bin/${CMAKE_CFG_INTDIR}/${_plugin_rel_dir}")
        set(_plugin_sync_target "osgi_sync_${_plugin_target}_runtime")

        set(_plugin_qml_dir "${_plugin_source_dir}/qml")
        if(EXISTS "${_plugin_qml_dir}")
            add_custom_target(${_plugin_sync_target}
                COMMAND ${CMAKE_COMMAND} -E make_directory "${_plugin_output_dir}"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${_plugin_json}"
                    "${_plugin_output_dir}/plugin.json"
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "${_plugin_qml_dir}"
                    "${_plugin_output_dir}/qml"
                VERBATIM
            )
        else()
            add_custom_target(${_plugin_sync_target}
                COMMAND ${CMAKE_COMMAND} -E make_directory "${_plugin_output_dir}"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${_plugin_json}"
                    "${_plugin_output_dir}/plugin.json"
                VERBATIM
            )
        endif()
        add_dependencies(${_plugin_sync_target} ${_plugin_target})
        set_property(GLOBAL APPEND PROPERTY OSGI_RUNTIME_PLUGIN_SYNC_TARGETS ${_plugin_sync_target})
    endforeach()
endfunction()

osgi_sync_plugin_metadata_to_runtime()
add_subdirectory(app)
get_property(_osgi_sync_targets GLOBAL PROPERTY OSGI_RUNTIME_PLUGIN_SYNC_TARGETS)
if(TARGET app AND _osgi_sync_targets)
    list(REMOVE_DUPLICATES _osgi_sync_targets)
    add_dependencies(app ${_osgi_sync_targets})
endif()
add_subdirectory(tools/field_simulator)

if(MSVC)
    set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT app)
endif()
