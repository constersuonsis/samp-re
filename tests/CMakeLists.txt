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

add_executable(transport_tests transport_tests.cpp)
target_link_libraries(transport_tests PRIVATE samp_client)

add_test(NAME transport COMMAND transport_tests)

add_executable(raknet_client_tests raknet_client_tests.cpp)
target_link_libraries(raknet_client_tests PRIVATE samp_client)

add_test(NAME raknet_client COMMAND raknet_client_tests)

add_executable(samp_client_tests samp_client_tests.cpp)
target_link_libraries(samp_client_tests PRIVATE samp_client)

add_test(NAME samp_client COMMAND samp_client_tests)

add_executable(player_pool_tests player_pool_tests.cpp)
target_link_libraries(player_pool_tests PRIVATE samp_client)

add_test(NAME player_pool COMMAND player_pool_tests)

add_executable(pool_tests pool_tests.cpp)
target_link_libraries(pool_tests PRIVATE samp_client)

add_test(NAME pool COMMAND pool_tests)

add_executable(spawn_controller_tests spawn_controller_tests.cpp)
target_link_libraries(spawn_controller_tests PRIVATE samp_client)

add_test(NAME spawn_controller COMMAND spawn_controller_tests)

add_executable(chat_window_tests chat_window_tests.cpp)
target_link_libraries(chat_window_tests PRIVATE samp_client)

add_test(NAME chat_window COMMAND chat_window_tests)

add_executable(dialog_tests dialog_tests.cpp)
target_link_libraries(dialog_tests PRIVATE samp_client)

add_test(NAME dialog COMMAND dialog_tests)

add_executable(text_draw_tests text_draw_tests.cpp)
target_link_libraries(text_draw_tests PRIVATE samp_client)

add_test(NAME text_draw COMMAND text_draw_tests)

add_executable(ui_misc_tests ui_misc_tests.cpp)
target_link_libraries(ui_misc_tests PRIVATE samp_client)

add_test(NAME ui_misc COMMAND ui_misc_tests)

add_executable(packet_logger_tests packet_logger_tests.cpp)
target_link_libraries(packet_logger_tests PRIVATE samp_client)

add_test(NAME packet_logger COMMAND packet_logger_tests)

add_executable(server_browser_tests server_browser_tests.cpp)
target_link_libraries(server_browser_tests PRIVATE samp_client)

add_test(NAME server_browser COMMAND server_browser_tests)

add_executable(player_sync_cache_tests player_sync_cache_tests.cpp)
target_link_libraries(player_sync_cache_tests PRIVATE samp_client)

add_test(NAME player_sync_cache COMMAND player_sync_cache_tests)
