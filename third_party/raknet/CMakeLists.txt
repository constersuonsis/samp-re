
add_library(raknet STATIC
    BitStream.cpp
    CheckSum.cpp
    DS_ByteQueue.cpp
    DS_HuffmanEncodingTree.cpp
    DataBlockEncryptor.cpp
    DataCompressor.cpp
    GetTime.cpp
    InternalPacketPool.cpp
    LinuxStrings.cpp
    NetworkIDGenerator.cpp
    NetworkTypes.cpp
    PluginInterface.cpp
    RPCMap.cpp
    RakClient.cpp
    RakNetStatistics.cpp
    RakNetworkFactory.cpp
    RakPeer.cpp
    RakSleep.cpp
    Rand.cpp
    ReliabilityLayer.cpp
    SHA1.cpp
    SimpleMutex.cpp
    SocketLayer.cpp
    StringCompressor.cpp
    StringTable.cpp
    SystemAddressList.cpp
    rijndael.cpp
    _findfirst.cpp
)

target_compile_definitions(raknet PRIVATE _RAKNET_LIB)
target_include_directories(raknet PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_include_directories(raknet SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

if(WIN32)
    target_link_libraries(raknet PUBLIC ws2_32)
endif()
