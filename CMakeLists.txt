cmake_minimum_required(VERSION 3.8)

project(test)

if (DEFINED EMSCRIPTEN)
	add_executable(main main.c)

	set(CMAKE_EXECUTABLE_SUFFIX ".html")

	set_target_properties(main PROPERTIES COMPILE_FLAGS "-Os -s SIDE_MODULE=1 ")
	set_target_properties(main PROPERTIES LINK_FLAGS    "-Os -s WASM=1 -s USE_WEBGL2 --shell-file ${CMAKE_SOURCE_DIR}/webpage/shell.html")
    target_include_directories(main PRIVATE sokol)
else()
	add_library(main SHARED main.c)
endif()