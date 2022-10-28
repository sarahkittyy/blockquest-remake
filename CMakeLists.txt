cmake_minimum_required(VERSION 3.13)
project(bq-r)
include(FetchContent)
include(ExternalProject)

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
find_package(OpenSSL REQUIRED)

# not libs
set(CMAKE_CXX_STANDARD 20)

file(GLOB_RECURSE sources "game/*.cpp")
file(GLOB imgui_sources "lib/imgui/*.cpp")

add_subdirectory(lib/jwt-cpp)

set(includes game/ lib/imgui/ lib/http lib/nlohmann lib/ lib/jwt-cpp/include ${OPENSSL_INCLUDE_DIR})
set(libs sfml-graphics sfml-window sfml-audio sfml-network sfml-system OpenGL::GL OpenAL jwt-cpp::jwt-cpp OpenSSL::SSL OpenSSL::Crypto)
set(flags -DCPPHTTPLIB_OPENSSL_SUPPORT)

if(NOT CMAKE_BUILD_TYPE MATCHES Debug)
	list(APPEND flags -DNDEBUG -DSERVER_URL="https://bq-r.sushicat.rocks")
else()
	list(APPEND flags -DSERVER_URL="http://localhost:3000")
endif()

if(WIN32)
	set(APP_ICON_RESOURCE_WINDOWS "${CMAKE_CURRENT_SOURCE_DIR}/appicon.rc")
	add_executable(bq-r WIN32 ${sources} ${imgui_sources} ${APP_ICON_RESOURCE_WINDOWS})
	list(APPEND libs sfml-main)
else()
	add_executable(bq-r ${sources} ${imgui_sources} ${APP_ICON_RESOURCE_WINDOWS})
endif()

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
		$<TARGET_FILE:sfml-main>
		$<TARGET_FILE:OpenSSL::SSL>
		$<TARGET_FILE:OpenSSL::Crypto>
		$<TARGET_FILE_DIR:bq-r>
	)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/openal32.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()
