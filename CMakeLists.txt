cmake_minimum_required(VERSION 3.13)
project(bq-r)
include(FetchContent)

# libs
set(SFML_VERSION "2.5.1")
FetchContent_Declare(
	sfml
	GIT_REPOSITORY "https://github.com/SFML/SFML.git"
	GIT_TAG "${SFML_VERSION}"
	)
FetchContent_GetProperties(sfml)
if(NOT sfml_POPULATED)
	FetchContent_Populate(sfml)
	add_subdirectory(${sfml_SOURCE_DIR} ${sfml_BINARY_DIR})
endif()

find_package(OpenGL REQUIRED)

# not libs
set(CMAKE_CXX_STANDARD 20)

file(GLOB_RECURSE sources "game/*.cpp")
file(GLOB imgui_sources "lib/imgui/*.cpp")

set(includes game/ lib/imgui/)
set(libs sfml-graphics sfml-window sfml-audio sfml-network sfml-system OpenGL::GL)
set(flags)

if(WIN32)
	set(APP_ICON_RESOURCE_WINDOWS "${CMAKE_CURRENT_SOURCE_DIR}/appicon.rc")
endif()

add_executable(bq-r ${sources} ${imgui_sources} ${APP_ICON_RESOURCE_WINDOWS})

target_compile_options(bq-r PUBLIC ${flags})
target_include_directories(bq-r PUBLIC ${includes})
target_link_libraries(bq-r ${libs})

if(WIN32)
	add_custom_command(TARGET bq-r POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
		$<TARGET_FILE:sfml-graphics>
		$<TARGET_FILE:sfml-window>
		$<TARGET_FILE:sfml-audio>
		$<TARGET_FILE:sfml-network>
		$<TARGET_FILE:sfml-system>
		$<TARGET_FILE_DIR:bq-r>
	)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()
