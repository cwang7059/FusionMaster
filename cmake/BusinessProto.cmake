include_guard(GLOBAL)

include(CMakeParseArguments)

function(osgi_business_collect_protobuf_link_libs out_var)
    set(_libs)

    if(WIN32)
        file(GLOB _libs "${PROTOBUF_LIB_DIR}/*.lib")
        list(FILTER _libs EXCLUDE REGEX "libprotoc\\.lib$")
    else()
        file(GLOB _libs
            "${PROTOBUF_LIB_DIR}/*.a"
            "${PROTOBUF_LIB_DIR}/*.so"
            "${PROTOBUF_LIB_DIR}/*.so.*"
        )
        list(FILTER _libs EXCLUDE REGEX "protoc")
    endif()

    list(LENGTH _libs _count)
    if(_count EQUAL 0)
        message(FATAL_ERROR "未在 ${PROTOBUF_LIB_DIR} 找到可用的 protobuf 链接库")
    endif()

    set(${out_var} ${_libs} PARENT_SCOPE)
endfunction()

function(osgi_business_add_proto target_name)
    set(options)
    set(oneValueArgs OUTPUT_DIR)
    set(multiValueArgs PROTO_FILES)
    cmake_parse_arguments(BP "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT BP_PROTO_FILES)
        message(FATAL_ERROR "osgi_business_add_proto(${target_name}) 缺少 PROTO_FILES")
    endif()

    if(NOT PROTOBUF_FOUND)
        message(FATAL_ERROR "当前未检测到 third_party/protobuf，无法生成 ${target_name}")
    endif()

    if(WIN32)
        set(_protoc "${PROTOBUF_BIN_DIR}/protoc.exe")
    else()
        set(_protoc "${PROTOBUF_BIN_DIR}/protoc")
    endif()

    if(NOT EXISTS "${_protoc}")
        message(FATAL_ERROR "未找到 protoc: ${_protoc}")
    endif()

    if(BP_OUTPUT_DIR)
        set(_gen_dir "${BP_OUTPUT_DIR}")
    else()
        set(_gen_dir "${CMAKE_CURRENT_BINARY_DIR}/${target_name}_generated")
    endif()
    file(MAKE_DIRECTORY "${_gen_dir}")

    set(_generated_sources)
    set(_generated_headers)

    foreach(proto_file ${BP_PROTO_FILES})
        if(IS_ABSOLUTE "${proto_file}")
            set(_proto_abs "${proto_file}")
        else()
            set(_proto_abs "${CMAKE_CURRENT_SOURCE_DIR}/${proto_file}")
        endif()

        if(NOT EXISTS "${_proto_abs}")
            message(FATAL_ERROR "proto 文件不存在: ${_proto_abs}")
        endif()

        get_filename_component(_proto_dir "${_proto_abs}" DIRECTORY)
        get_filename_component(_proto_name "${_proto_abs}" NAME)
        get_filename_component(_proto_stem "${_proto_abs}" NAME_WE)

        list(APPEND _generated_sources "${_gen_dir}/${_proto_stem}.pb.cc")
        list(APPEND _generated_headers "${_gen_dir}/${_proto_stem}.pb.h")

        add_custom_command(
            OUTPUT
                "${_gen_dir}/${_proto_stem}.pb.cc"
                "${_gen_dir}/${_proto_stem}.pb.h"
            COMMAND "${_protoc}"
                --cpp_out=${_gen_dir}
                --proto_path=${_proto_dir}
                ${_proto_name}
            WORKING_DIRECTORY
                "${_proto_dir}"
            DEPENDS
                "${_proto_abs}"
            COMMENT "生成 Protobuf 代码: ${_proto_name}"
            VERBATIM
        )
    endforeach()

    osgi_business_collect_protobuf_link_libs(_protobuf_link_libs)

    add_library(${target_name} STATIC
        ${_generated_sources}
        ${_generated_headers}
    )

    target_include_directories(${target_name}
        PUBLIC
            "${_gen_dir}"
            "${PROTOBUF_INCLUDE_DIR}"
    )

    target_link_libraries(${target_name}
        PUBLIC
            ${_protobuf_link_libs}
    )

    target_compile_definitions(${target_name}
        PUBLIC
            OSGI_PROTOBUF_AVAILABLE=1
            PROTOBUF_USE_DLLS=1
    )
endfunction()
