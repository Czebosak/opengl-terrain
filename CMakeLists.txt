cmake_minimum_required(VERSION "3.19.2")

project(Terrain)
set(CMAKE_CXX_STANDARD 23)

add_subdirectory("external/haygl")

file(GLOB_RECURSE MY_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_executable(Terrain ${MY_SOURCES}) 

target_include_directories(Terrain PRIVATE "src")
target_include_directories(Terrain PRIVATE "external/haygl/src")
target_include_directories(Terrain PRIVATE "external/fastnoiselite")

target_link_libraries(Terrain PRIVATE HayGL)
