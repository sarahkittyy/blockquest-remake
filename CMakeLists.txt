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

FetchContent_Declare(
	imgui
	GIT_REPOSITORY https://github.com/ocornut/imgui
)
FetchContent_MakeAvailable(imgui)

FetchContent_Declare(
	imgui-sfml
	GIT_REPOSITORY https://github.com/hulakdar/imgui-sfml
)
#add_subdirectory(imgui-sfml)
set(IMGUI_DIR ${imgui_SOURCE_DIR})
set(IMGUI_SFML_FIND_SFML OFF)
set(IMGUI_SFML_IMGUI_DEMO ON)
FetchContent_MakeAvailable(imgui-sfml)

# not libs
set(CMAKE_CXX_STANDARD 20)

file(GLOB_RECURSE sources "game/*.cpp")

set(includes "game/ lib/ lib/Box2D/Box2D")
set(libs sfml-graphics sfml-window sfml-audio sfml-network sfml-system ImGui-SFML::ImGui-SFML)
set(flags -Wall -Wno-unused -Wno-switch -Wno-int-to-void-pointer-cast)

add_executable(bq-r ${sources})

target_compile_options(bq-r PUBLIC ${flags})
target_include_directories(bq-r PUBLIC ${includes})
target_link_libraries(bq-r ${libs})
