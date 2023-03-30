cmake_minimum_required(VERSION 3.18)
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

set(OPENSSL_USE_STATIC_LIBS TRUE)
find_package(OpenSSL REQUIRED)
find_package(sioclient REQUIRED)

# not libs
set(CMAKE_CXX_STANDARD 20)

file(GLOB_RECURSE sources "game/*.cpp")
file(GLOB imgui_sources "lib/imgui/*.cpp")

set(filedialog_sources "lib/ImGuiFileDialog/ImGuiFileDialog.cpp")

set(flags -DCPPHTTPLIB_OPENSSL_SUPPORT -DSIO_TLS=1)

if(NOT CMAKE_BUILD_TYPE MATCHES "Debug")
	list(APPEND flags -DNDEBUG -DSERVER_URL="https://bq-r.sushicat.rocks")
else()
	list(APPEND flags -DNO_VERIFY_CERTS -DSERVER_URL="https://localhost:3000")
endif()

set(includes game/ lib/imgui/ lib/http lib/nlohmann lib/ImGuiFileDialog lib/ ${OPENSSL_INCLUDE_DIR})
set(libs sfml-graphics sfml-window sfml-audio sfml-network sfml-system OpenGL::GL OpenAL OpenSSL::SSL OpenSSL::Crypto sioclient::sioclient_tls)

if(APP_VERSION)
	list(APPEND flags "-DAPP_TAG=${APP_VERSION}")
endif()

if(WIN32)
	list(APPEND includes lib/ImGuiFileDialog/dirent)
	set(APP_ICON_RESOURCE_WINDOWS "${CMAKE_CURRENT_SOURCE_DIR}/appicon.rc")
	add_executable(bq-r WIN32 ${sources} ${imgui_sources} ${filedialog_sources} ${APP_ICON_RESOURCE_WINDOWS})
	list(APPEND libs sfml-main)
else()
	add_executable(bq-r ${sources} ${imgui_sources} ${filedialog_sources} ${APP_ICON_RESOURCE_WINDOWS})
endif()

target_compile_options(bq-r PUBLIC ${flags})
target_include_directories(bq-r PUBLIC ${includes})
target_link_libraries(bq-r PRIVATE ${libs})
if (WIN32)
	target_link_libraries(bq-r PRIVATE OpenSSL::applink)
endif()

if(WIN32)
	add_custom_command(TARGET bq-r POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
		$<TARGET_FILE:sfml-graphics>
		$<TARGET_FILE:sfml-window>
		$<TARGET_FILE:sfml-audio>
		$<TARGET_FILE:sfml-network>
		$<TARGET_FILE:sfml-system>
		$<TARGET_FILE:sfml-main>
		$<TARGET_FILE_DIR:bq-r>
	)

	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/openal32.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()
