cmake_minimum_required(VERSION 3.20)

project(samp-client-r3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(SAMP_BUILD_TESTS "Build unit tests" ON)

add_subdirectory(third_party/raknet)

add_library(samp_client STATIC
    src/client/startup.cpp
    src/client/init_thread.cpp
    src/client/settings.cpp
    src/client/controller.cpp
    src/client/session.cpp
    src/client/network_client.cpp
    src/client/security_archive.cpp
    src/client/tick.cpp
    src/client/spawn.cpp
    src/player/player_colors.cpp
    src/player/player_settings.cpp
    src/audio/audio.cpp
    src/game/boot.cpp
    src/game/memory.cpp
    src/game/version.cpp
    src/game/tables.cpp
    src/game/limits.cpp
    src/game/hooks.cpp
    src/game/detour.cpp
    src/game/length_decoder.cpp
    src/game/frame.cpp
    src/game/memory_patch.cpp
    src/game/anticheat.cpp
    src/game/vehicle_preview.cpp
    src/game/animation_ids.cpp
    src/game/camera_presets.cpp
    src/game/weapon_presets.cpp
    src/game/patches.cpp
    src/scripting/command_manager.cpp
    src/ui/crash_dialog.cpp
    src/ui/render_device.cpp
    src/ui/font.cpp
    src/ui/chat.cpp
    src/ui/renderer.cpp
    src/ui/textdraw.cpp
    src/ui/scoreboard.cpp
    src/ui/dialog.cpp
    src/ui/timer.cpp
    src/ui/game_window.cpp
    src/ui/display.cpp
    src/ui/world_ui.cpp
    src/scripting/debug_script.cpp
    src/util/ini_store.cpp
    src/util/logger.cpp
    src/util/filesystem.cpp
    src/util/precise_time.cpp
)

target_include_directories(samp_client PUBLIC include)
target_link_libraries(samp_client PUBLIC raknet winmm)

if(MSVC)
    target_compile_options(samp_client PRIVATE /W4)
else()
    target_compile_options(samp_client PRIVATE -Wall -Wextra)
endif()

add_library(samp SHARED src/client/dll_main.cpp)
target_link_libraries(samp PRIVATE samp_client)
target_include_directories(samp PRIVATE include)

set(SAMP_DEPLOY_DIR "C:/gta/cherepash gta v4" CACHE PATH "GTA folder receiving samp.dll")
add_custom_command(TARGET samp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${SAMP_DEPLOY_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:samp> "${SAMP_DEPLOY_DIR}/samp.dll")
