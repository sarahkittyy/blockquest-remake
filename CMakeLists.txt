cmake_minimum_required(VERSION 3.13)
project(bq-r)
include(FetchContent)

# libs
find_package(SFML 2.5 COMPONENTS graphics window audio network system)

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
set(flags -Wall -Wno-unused -Wno-switch)

add_executable(bq-r ${sources})

target_compile_options(bq-r PUBLIC ${flags})
target_include_directories(bq-r PUBLIC ${includes})
target_link_libraries(bq-r ${libs})
