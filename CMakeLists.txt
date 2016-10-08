cmake_minimum_required(VERSION 2.8.12)
project(CardboardPlugin)

find_package(osvr REQUIRED)
find_package(jsoncpp REQUIRED)
find_package(protobuf REQUIRED)

osvr_convert_json(je_nourish_cardboard_json
    src/je_nourish_cardboard.json
    "${CMAKE_CURRENT_BINARY_DIR}/je_nourish_cardboard_json.h")

osvr_convert_json(display_descriptor
    src/display_descriptor.json
    "${CMAKE_CURRENT_BINARY_DIR}/display_descriptor.h")

include_directories("${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/src" "${PROTOBUF_INCLUDE_DIR}")

PROTOBUF_GENERATE_CPP(ProtoSources ProtoHeaders src/CardboardDevice.proto)

file(GLOB CARDBOARD_SOURCES src/*.cpp)
file(GLOB CARDBOARD_HEADERS src/*.h)

osvr_add_plugin(NAME je_nourish_cardboard
    CPP 
    SOURCES
	${CARDBOARD_SOURCES}
	${CARDBOARD_HEADERS}

	src/je_nourish_cardboard.rc
	src/CardboardDevice.proto

	${ProtoSources}
	${ProtoHeaders}

    "${CMAKE_CURRENT_BINARY_DIR}/je_nourish_cardboard_json.h"
	"${CMAKE_CURRENT_BINARY_DIR}/display_descriptor.h")

target_link_libraries(je_nourish_cardboard ${PROTOBUF_LIBRARIES} jsoncpp_lib)