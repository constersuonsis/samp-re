cmake_minimum_required(VERSION 3.20)

project(samp-client-r3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(SAMP_BUILD_TESTS "Build unit tests" ON)

add_subdirectory(third_party/raknet)

add_library(samp_client STATIC
    src/network/bit_stream.cpp
    src/network/packet_logger.cpp
    src/network/raknet_client.cpp
    src/network/server_browser.cpp
    src/objects/object_pool.cpp
    src/objects/object_pool_sync.cpp
    src/player/player_pool.cpp
    src/player/player_pool_sync.cpp
    src/player/player_sync_cache.cpp
    src/protocol/sync_codec.cpp
    src/protocol/protocol_string.cpp
    src/protocol/rpc_payloads.cpp
    src/protocol/rpc_dispatcher.cpp
    src/protocol/packet_reader.cpp
    src/protocol/sync_throttle.cpp
    src/protocol/sync_interpolation.cpp
    src/protocol/samp_client.cpp
    src/protocol/spawn_controller.cpp
    src/ui/chat_window.cpp
    src/ui/dialog.cpp
    src/ui/scoreboard.cpp
    src/ui/text_draw.cpp
    src/util/command_processor.cpp
    src/compression/huffman_tree.cpp
    src/crypto/xtea.cpp
    src/crypto/packet_checksum.cpp
    src/crypto/packet_codec.cpp
    src/crypto/aes128.cpp
    src/crypto/rsa.cpp
    src/vehicle/vehicle_pool.cpp
    src/vehicle/vehicle_pool_sync.cpp
    src/vehicle/vehicle_sync_cache.cpp
)

target_include_directories(samp_client PUBLIC include)
target_link_libraries(samp_client PUBLIC raknet)

if(MSVC)
    target_compile_options(samp_client PRIVATE /W4)
else()
    target_compile_options(samp_client PRIVATE -Wall -Wextra)
endif()

option(SAMP_CHECK_HEADERS "Verify that public headers compile standalone" ON)

if(SAMP_CHECK_HEADERS)
    file(GLOB_RECURSE samp_public_headers
         RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/include
         include/*.h)

    set(samp_header_check_sources "")
    foreach(header ${samp_public_headers})
        string(REGEX REPLACE "[/.]" "_" check_name "${header}")
        set(generated "${CMAKE_CURRENT_BINARY_DIR}/header_checks/${check_name}.cpp")
        file(GENERATE OUTPUT "${generated}" CONTENT "#include \"${header}\"\n")
        list(APPEND samp_header_check_sources "${generated}")
    endforeach()

    add_library(samp_header_checks OBJECT ${samp_header_check_sources})
    target_include_directories(samp_header_checks PRIVATE include)
endif()

if(SAMP_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
