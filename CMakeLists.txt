cmake_minimum_required(VERSION 3.26)
project(dusklight_twilit_essentials CXX)

# The Dusklight version to build against: a release tag (e.g. "v1.5.0") or a commit SHA.
if (NOT DUSKLIGHT_VERSION)
    set(DUSKLIGHT_VERSION "d281c34a65568ee7be2319985f3a290c0ef65abc")
endif ()

# Fetches the Dusklight sources into ./dusklight and provides DUSKLIGHT_DIR.
# Use -DDUSKLIGHT_DIR= to point at an existing Dusklight checkout.
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/FetchDusklight.cmake")
add_subdirectory("${DUSKLIGHT_DIR}/sdk" dusklight-sdk EXCLUDE_FROM_ALL)

# Read version from mod.json automatically
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/mod.json" MOD_JSON_CONTENT)
string(JSON TWILIT_ESSENTIALS_VER GET "${MOD_JSON_CONTENT}" "version")

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/mod.json")
set_source_files_properties(src/mod.cpp src/update_service.cpp PROPERTIES OBJECT_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/mod.json")

# Automatically copy textures directory into res/textures so they get packaged into .dusk
add_mod(dusklight_twilit_essentials
        FEATURES game
        SOURCES 
            src/mod.cpp
            src/hp_bars/hp_bars.cpp
            src/visible_equipment/visible_equipment.cpp
            src/z_button/z_button.cpp
            src/horse_call/horse_call.cpp
            src/sheathed_spin/sheathed_spin.cpp
            src/puppet_zelda_pattern/puppet_zelda_pattern.cpp
            src/update_service.cpp
            src/collection_menu/collection_menu.cpp
        MOD_JSON mod.json
        RES_DIR res
        TEXTURES_DIR textures
)

target_compile_definitions(dusklight_twilit_essentials PRIVATE TWILIT_ESSENTIALS_VERSION="${TWILIT_ESSENTIALS_VER}")

# Automatically sync version in README.md on every build
add_custom_command(TARGET dusklight_twilit_essentials POST_BUILD
    COMMAND ${CMAKE_COMMAND}
        -DMOD_JSON_PATH="${CMAKE_CURRENT_SOURCE_DIR}/mod.json"
        -DREADME_PATH="${CMAKE_CURRENT_SOURCE_DIR}/README.md"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/SyncReadmeVersion.cmake"
)

# Prevent MSVC linker from stripping mod metadata section records (/OPT:REF)
if (MSVC)
    target_link_options(dusklight_twilit_essentials PRIVATE "/OPT:NOREF")
endif ()

# Automatically copy the built .dusk archive to the user mods directory after every build (Windows only)
if (WIN32 AND EXISTS "$ENV{APPDATA}")
    set(DUSKLIGHT_USER_MODS_DIR "$ENV{APPDATA}/TwilitRealm/Dusklight/mods")

    add_custom_command(TARGET dusklight_twilit_essentials_package POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${DUSKLIGHT_USER_MODS_DIR}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${CMAKE_BINARY_DIR}/mods/dusklight_twilit_essentials.dusk"
                    "${DUSKLIGHT_USER_MODS_DIR}/dusklight_twilit_essentials.dusk"
            COMMENT "Copying dusklight_twilit_essentials.dusk to ${DUSKLIGHT_USER_MODS_DIR}"
    )
endif ()

# Automatically copy the built .dusk archive to the user mods directory after every build (macOS only)
if (APPLE)
    set(DUSKLIGHT_USER_MODS_DIR "$ENV{HOME}/Library/Application Support/TwilitRealm/Dusklight/mods")

    add_custom_command(TARGET dusklight_twilit_essentials_package POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${DUSKLIGHT_USER_MODS_DIR}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${CMAKE_BINARY_DIR}/mods/dusklight_twilit_essentials.dusk"
                    "${DUSKLIGHT_USER_MODS_DIR}/dusklight_twilit_essentials.dusk"
            COMMENT "Copying dusklight_twilit_essentials.dusk to ${DUSKLIGHT_USER_MODS_DIR}"
    )
endif ()

# Automatically copy the built .dusk archive to the user mods directory after every build (Linux only)
if (UNIX AND NOT APPLE)
    set(DUSKLIGHT_USER_MODS_DIR "$ENV{HOME}/.local/share/TwilitRealm/Dusklight/mods")

    add_custom_command(TARGET dusklight_twilit_essentials_package POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${DUSKLIGHT_USER_MODS_DIR}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${CMAKE_BINARY_DIR}/mods/dusklight_twilit_essentials.dusk"
                    "${DUSKLIGHT_USER_MODS_DIR}/dusklight_twilit_essentials.dusk"
            COMMENT "Copying dusklight_twilit_essentials.dusk to ${DUSKLIGHT_USER_MODS_DIR}"
    )
endif ()
