cmake_minimum_required(VERSION 3.26)
project(dusklight_twilit_essentials CXX)

# The Dusklight version to build against: a release tag (e.g. "v1.5.0") or a commit SHA.
if (NOT DUSKLIGHT_VERSION)
    set(DUSKLIGHT_VERSION "8caab1a6ba4aad5513bc9fe402a76dce53964e9c")
endif ()

# Fetches the Dusklight sources into ./dusklight and provides DUSKLIGHT_DIR.
# Use -DDUSKLIGHT_DIR= to point at an existing Dusklight checkout.
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/FetchDusklight.cmake")
add_subdirectory("${DUSKLIGHT_DIR}/sdk" dusklight-sdk EXCLUDE_FROM_ALL)

# Automatically copy textures directory into res/textures so they get packaged into .dusk
add_mod(dusklight_twilit_essentials
        FEATURES game
        SOURCES 
            src/mod.cpp
            src/hp_bars/hp_bars.cpp
            src/visible_equipment/visible_equipment.cpp
            src/damage_numbers/damage_numbers.cpp
            src/custom_z_button/custom_z_button.cpp
            src/horse_call/horse_call.cpp
        MOD_JSON mod.json
        RES_DIR res
        OVERLAY_DIR overlay
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
