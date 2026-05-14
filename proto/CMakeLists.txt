set(OSGI_PROTO_FILE "${CMAKE_CURRENT_SOURCE_DIR}/messages.proto")
set(OSGI_PROTO_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(OSGI_PROTO_SRC "${OSGI_PROTO_GEN_DIR}/messages.pb.cc")
set(OSGI_PROTO_HDR "${OSGI_PROTO_GEN_DIR}/messages.pb.h")

set(OSGI_PROTOC_EXECUTABLE "")
if(WIN32)
    set(OSGI_PROTOC_EXECUTABLE "${PROTOBUF_BIN_DIR}/protoc.exe")
else()
    set(OSGI_PROTOC_EXECUTABLE "${PROTOBUF_BIN_DIR}/protoc")
endif()

if(PROTOBUF_FOUND AND EXISTS "${OSGI_PROTOC_EXECUTABLE}")
    file(MAKE_DIRECTORY "${OSGI_PROTO_GEN_DIR}")

    add_custom_command(
        OUTPUT
            "${OSGI_PROTO_SRC}"
            "${OSGI_PROTO_HDR}"
        COMMAND "${OSGI_PROTOC_EXECUTABLE}"
            --cpp_out=${OSGI_PROTO_GEN_DIR}
            --proto_path=${CMAKE_CURRENT_SOURCE_DIR}
            messages.proto
        WORKING_DIRECTORY
            "${CMAKE_CURRENT_SOURCE_DIR}"
        DEPENDS
            "${OSGI_PROTO_FILE}"
        COMMENT "生成 Protobuf 代码: messages.proto"
        VERBATIM
    )

    add_library(proto_messages SHARED
        "${OSGI_PROTO_SRC}"
        "${OSGI_PROTO_HDR}"
    )

    set_target_properties(proto_messages PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS ON
        POSITION_INDEPENDENT_CODE ON
    )

    target_include_directories(proto_messages
        PUBLIC
            "${OSGI_PROTO_GEN_DIR}"
            "${PROTOBUF_INCLUDE_DIR}"
    )

    set(OSGI_PROTOBUF_LINK_LIBS)
    if(WIN32)
        file(GLOB OSGI_PROTOBUF_LINK_LIBS "${PROTOBUF_LIB_DIR}/*.lib")
        list(FILTER OSGI_PROTOBUF_LINK_LIBS EXCLUDE REGEX "libprotoc\\.lib$")
    else()
        file(GLOB OSGI_PROTOBUF_LINK_LIBS
            "${PROTOBUF_LIB_DIR}/*.a"
            "${PROTOBUF_LIB_DIR}/*.so"
            "${PROTOBUF_LIB_DIR}/*.so.*"
        )
        list(FILTER OSGI_PROTOBUF_LINK_LIBS EXCLUDE REGEX "protoc")
    endif()

    list(LENGTH OSGI_PROTOBUF_LINK_LIBS OSGI_PROTOBUF_LINK_LIBS_COUNT)
    if(OSGI_PROTOBUF_LINK_LIBS_COUNT EQUAL 0)
        message(FATAL_ERROR "proto: 未找到可用的 protobuf 链接库，请检查 ${PROTOBUF_LIB_DIR}")
    endif()

    target_link_libraries(proto_messages
        PUBLIC
            ${OSGI_PROTOBUF_LINK_LIBS}
    )

    target_compile_definitions(proto_messages
        PUBLIC OSGI_PROTOBUF_AVAILABLE=1 PROTOBUF_USE_DLLS=1
    )

    message(STATUS "proto: 使用 protoc ${OSGI_PROTOC_EXECUTABLE}")
    message(STATUS "proto: 链接库数量 ${OSGI_PROTOBUF_LINK_LIBS_COUNT}")
else()
    add_library(proto_messages INTERFACE)
    target_compile_definitions(proto_messages
        INTERFACE OSGI_PROTOBUF_AVAILABLE=0
    )
    message(WARNING "proto: third_party/protobuf 不完整，使用无 Protobuf 兜底模式")
endif()

