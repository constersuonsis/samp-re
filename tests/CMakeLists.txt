add_executable(bit_stream_tests bit_stream_tests.cpp)
target_link_libraries(bit_stream_tests PRIVATE samp_client)

add_test(NAME bit_stream COMMAND bit_stream_tests)

add_executable(sync_structures_tests sync_structures_tests.cpp)
target_link_libraries(sync_structures_tests PRIVATE samp_client)

add_test(NAME sync_structures COMMAND sync_structures_tests)

add_executable(sync_codec_tests sync_codec_tests.cpp)
target_link_libraries(sync_codec_tests PRIVATE samp_client)

add_test(NAME sync_codec COMMAND sync_codec_tests)

add_executable(rpc_ids_tests rpc_ids_tests.cpp)
target_link_libraries(rpc_ids_tests PRIVATE samp_client)

add_test(NAME rpc_ids COMMAND rpc_ids_tests)

add_executable(rpc_payloads_tests rpc_payloads_tests.cpp)
target_link_libraries(rpc_payloads_tests PRIVATE samp_client)

add_test(NAME rpc_payloads COMMAND rpc_payloads_tests)

add_executable(huffman_tree_tests huffman_tree_tests.cpp)
target_link_libraries(huffman_tree_tests PRIVATE samp_client)

add_test(NAME huffman_tree COMMAND huffman_tree_tests)

add_executable(crypto_tests crypto_tests.cpp)
target_link_libraries(crypto_tests PRIVATE samp_client)

add_test(NAME crypto COMMAND crypto_tests)

add_executable(rpc_dispatcher_tests rpc_dispatcher_tests.cpp)
target_link_libraries(rpc_dispatcher_tests PRIVATE samp_client)

add_test(NAME rpc_dispatcher COMMAND rpc_dispatcher_tests)

add_executable(packet_reader_tests packet_reader_tests.cpp)
target_link_libraries(packet_reader_tests PRIVATE samp_client)

add_test(NAME packet_reader COMMAND packet_reader_tests)

add_executable(sync_throttle_tests sync_throttle_tests.cpp)
target_link_libraries(sync_throttle_tests PRIVATE samp_client)

add_test(NAME sync_throttle COMMAND sync_throttle_tests)

add_executable(sync_interpolation_tests sync_interpolation_tests.cpp)
target_link_libraries(sync_interpolation_tests PRIVATE samp_client)

add_test(NAME sync_interpolation COMMAND sync_interpolation_tests)

add_executable(parser_robustness_tests parser_robustness_tests.cpp)
target_link_libraries(parser_robustness_tests PRIVATE samp_client)

add_test(NAME parser_robustness COMMAND parser_robustness_tests)
