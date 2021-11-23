cmake_minimum_required(VERSION 3.19)

add_compile_definitions(_ROOT_DIR="${CMAKE_SOURCE_DIR}")

project(VIGOR)
add_subdirectory(glad)
add_subdirectory(src)

set_property(TARGET vigor PROPERTY CXX_STANDARD 20)

find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(freetype CONFIG REQUIRED)

target_link_libraries(vigor PUBLIC glfw)
target_link_libraries(vigor PUBLIC freetype)