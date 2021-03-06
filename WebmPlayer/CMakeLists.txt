cmake_minimum_required (VERSION 3.0.0 FATAL_ERROR)

project (webm_player C CXX)

set(WEBM_PLAYER_VERSION_MAJOR 1)
set(WEBM_PLAYER_VERSION_MINOR 0)
set(WEBM_PLAYER_VERSION_PATCH 0)
set(PROJECT_VERSION ${WEBM_PLAYER_VERSION_MAJOR}.${WEBM_PLAYER_VERSION_MINOR}.${WEBM_PLAYER_VERSION_PATCH})

set(CMAKE_DEBUG_POSTFIX  "_d")

set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE) 

set(WEBM_PLAYER_SOURCE_PATH ${PROJECT_SOURCE_DIR}/src)
set(WEBM_PLAYER_INCLUDE_PATH ${PROJECT_SOURCE_DIR}/include)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

message(STATUS "Host: ${CMAKE_HOST_SYSTEM_PROCESSOR}")

if(NOT DEFINED WEBM_PLAYER_ARCHITECTURE)
    if(${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "AMD64")
        set(WEBM_PLAYER_ARCHITECTURE "x64" CACHE FILEPATH "Architecture")
    else()
        set(WEBM_PLAYER_ARCHITECTURE "x86" CACHE FILEPATH "Architecture")
    endif()
endif()

# Require libWebmPlayer runtime directory
set(LIB_WEBM_PLAYER_RUNTIME_PATH ${PROJECT_SOURCE_DIR}/runtime CACHE PATH "")
if(LIB_WEBM_PLAYER_RUNTIME_PATH STREQUAL "")
message(WARNING "Please define libWebmPlayer runtime path as CMake argument!\n-DLIB_WEBM_PLAYER_RUNTIME_PATH=path/to/libWebmPlayer/runtime")
endif()

set(WEBM_PLAYER_RUNTIME_PATH ${PROJECT_SOURCE_DIR}/runtime)
set(WEBM_PLAYER_EXAMPLE_PATH ${PROJECT_SOURCE_DIR}/example)

message(STATUS "WEBM_PLAYER_ARCHITECTURE: " ${WEBM_PLAYER_ARCHITECTURE})

if(WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3 /EHsc /MT")
    set(CMAKE_CXX_FLAGS_DEBUG          "${CMAKE_CXX_FLAGS} /Od /Ob0 /Zi /MTd")
    set(CMAKE_CXX_FLAGS_MINSIZEREL     "${CMAKE_CXX_FLAGS} /O1 /Ob2 /DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE        "${CMAKE_CXX_FLAGS} /Ox /Ob2 /Ot /DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS} /Od /Ob0 /Zi")

    if(${WEBM_PLAYER_ARCHITECTURE} STREQUAL "x64")
        set(WEBM_PLAYER_RUNTIME_PATH "${WEBM_PLAYER_RUNTIME_PATH}/win64")
		set(VPX_RUNTIME_PATH "${LIB_WEBM_PLAYER_RUNTIME_PATH}/win64")
    else()
        set(WEBM_PLAYER_RUNTIME_PATH "${WEBM_PLAYER_RUNTIME_PATH}/win32")
		set(VPX_RUNTIME_PATH "${LIB_WEBM_PLAYER_RUNTIME_PATH}/win32")
    endif()
endif()

message(STATUS "Source path: " ${PROJECT_SOURCE_DIR})
message(STATUS "Include path: " ${WEBM_PLAYER_INCLUDE_PATH})
message(STATUS "Binary path: " ${PROJECT_BINARY_DIR})
message(STATUS "CXX flags: " ${CMAKE_CXX_FLAGS})
message(STATUS "Runtime path: " ${WEBM_PLAYER_RUNTIME_PATH})
message(STATUS "CMAKE_CURRENT_BINARY_DIR: " ${CMAKE_CURRENT_BINARY_DIR})
message(STATUS "CMAKE_BUILD_TYPE: " ${CMAKE_BUILD_TYPE})

# Include headers
include_directories(BEFORE SYSTEM ${WEBM_PLAYER_INCLUDE_PATH})
include_directories(BEFORE SYSTEM ${WEBM_PLAYER_INCLUDE_PATH}/3rd)
include_directories(BEFORE SYSTEM ${WEBM_PLAYER_RUNTIME_PATH}/include)
include_directories(BEFORE SYSTEM ${VPX_RUNTIME_PATH}/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Environment
include(TestForANSIForScope)
include(TestForANSIStreamHeaders)
include(TestForSTDNamespace)

message(STATUS "CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")

set(CMAKE_PACKAGE_QTGUI TRUE)

set(SOURCES
${WEBM_PLAYER_SOURCE_PATH}/main.cpp
${WEBM_PLAYER_SOURCE_PATH}/webm_player.cpp
)

set(HEADERS
${WEBM_PLAYER_INCLUDE_PATH}/webm_player.hpp
)

set(ALL_SOURCES ${SOURCES} ${HEADERS})

add_definitions(
-DWEBM_PLAYER_VERSION="${PROJECT_VERSION}"
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

set(EXECUTABLE_NAME "webm_player")

add_executable(${EXECUTABLE_NAME} WIN32 ${ALL_SOURCES})

set_target_properties(${EXECUTABLE_NAME} PROPERTIES DEBUG_POSTFIX _d)
set_target_properties(${EXECUTABLE_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

set(CONFIG_DIR $<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>)

if(WIN32)
	set_property(TARGET ${EXECUTABLE_NAME} PROPERTY WIN32_EXECUTABLE 0)
endif()

if(WIN32)
    target_link_libraries(${EXECUTABLE_NAME}
        ${WEBM_PLAYER_RUNTIME_PATH}/lib/SDL2main.lib
        ${WEBM_PLAYER_RUNTIME_PATH}/lib/SDL2.lib
		
		optimized ${VPX_RUNTIME_PATH}/lib/libWebmPlayer.lib
		debug ${VPX_RUNTIME_PATH}/lib/libWebmPlayer_d.lib
		)
		
		add_custom_target(INSTALL_AFTER_ALL ALL COMMAND ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/cmake_install.cmake)

		add_custom_command(TARGET INSTALL_AFTER_ALL POST_BUILD
		COMMAND ${CMAKE_COMMAND} ARGS -E copy "${WEBM_PLAYER_RUNTIME_PATH}/bin/SDL2.dll" "${CMAKE_CURRENT_BINARY_DIR}/bin/${CONFIG_DIR}/SDL2.dll")
		
		add_custom_command(TARGET INSTALL_AFTER_ALL POST_BUILD
		COMMAND ${CMAKE_COMMAND} ARGS -E copy "${WEBM_PLAYER_EXAMPLE_PATH}/run$<$<CONFIG:Debug>:_d>.bat" "${CMAKE_CURRENT_BINARY_DIR}/bin/${CONFIG_DIR}/run$<$<CONFIG:Debug>:_d>.bat")
		
		add_custom_command(TARGET INSTALL_AFTER_ALL POST_BUILD
		COMMAND ${CMAKE_COMMAND} ARGS -E copy "$<$<CONFIG:debug>:${WEBM_PLAYER_RUNTIME_PATH}/bin/libWebmPlayer_d.dll>$<$<CONFIG:release>:${WEBM_PLAYER_RUNTIME_PATH}/bin/libWebmPlayer.dll>" "${CMAKE_CURRENT_BINARY_DIR}/bin/${CONFIG_DIR}")
endif()

add_custom_command(TARGET INSTALL_AFTER_ALL POST_BUILD
COMMAND ${CMAKE_COMMAND} ARGS -E copy "${WEBM_PLAYER_EXAMPLE_PATH}/trailer_1080p.webm" "${CMAKE_CURRENT_BINARY_DIR}/bin/${CONFIG_DIR}/trailer_1080p.webm")

add_custom_command(TARGET INSTALL_AFTER_ALL POST_BUILD
COMMAND ${CMAKE_COMMAND} ARGS -E copy "${WEBM_PLAYER_EXAMPLE_PATH}/trailer_license.txt" "${CMAKE_CURRENT_BINARY_DIR}/bin/${CONFIG_DIR}/trailer_license.txt")