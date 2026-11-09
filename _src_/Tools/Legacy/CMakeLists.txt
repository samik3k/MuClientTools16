# MuLegacy for macOS / Linux (Windows builds use MuLegacy.vcxproj from ClientTools16.sln).
#   cmake -S _src_/Tools/Legacy -B build && cmake --build build
# The exe is copied to _bin_/Legacy, next to the LegacyFormats schema folder.
cmake_minimum_required(VERSION 3.13)
project(MuLegacy CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

set(LEGACY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../Legacy)
add_executable(MuLegacy
	MuLegacy/Main.cpp
	${LEGACY_DIR}/LegacyCommon.cpp
	${LEGACY_DIR}/LegacyGraphic.cpp
	${LEGACY_DIR}/LegacySchema.cpp
	${LEGACY_DIR}/LegacyTerrain.cpp)
target_include_directories(MuLegacy PRIVATE ${LEGACY_DIR})

add_custom_command(TARGET MuLegacy POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:MuLegacy> ${CMAKE_CURRENT_SOURCE_DIR}/../../../_bin_/Legacy/)
