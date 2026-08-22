cmake_minimum_required(VERSION 3.20)

project(samp-client-r3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(SAMP_BUILD_TESTS "Build unit tests" ON)

add_library(samp_client STATIC
    src/network/bit_stream.cpp
    src/protocol/sync_codec.cpp
    src/protocol/protocol_string.cpp
    src/protocol/rpc_payloads.cpp
    src/protocol/rpc_dispatcher.cpp
    src/protocol/packet_reader.cpp
    src/protocol/sync_throttle.cpp
    src/protocol/sync_interpolation.cpp
    src/compression/huffman_tree.cpp
    src/crypto/xtea.cpp
    src/crypto/packet_checksum.cpp
    src/crypto/packet_codec.cpp
)

target_include_directories(samp_client PUBLIC include)

if(MSVC)
    target_compile_options(samp_client PRIVATE /W4)
else()
    target_compile_options(samp_client PRIVATE -Wall -Wextra)
endif()

if(SAMP_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
