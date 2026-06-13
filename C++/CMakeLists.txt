cmake_minimum_required(VERSION 3.20)
project(http_server  CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)

    # Using ENV for dev purposes
    # Replace this with path to local copy of AdonisCode73/cpp-net
add_subdirectory(
    $ENV{CPP_NET_PATH}
    ${CMAKE_BINARY_DIR}/cpp-net-build
)

add_executable(http_server  ${SOURCES})

target_link_libraries(http_server PRIVATE net)

target_compile_options(http_server  PRIVATE
    -Wall -Wextra -Wpedantic
    $<$<CONFIG:Debug>:-fsanitize=address,undefined -g>
    $<$<CONFIG:Release>:-O3>)
target_link_options(http_server  PRIVATE
    $<$<CONFIG:Debug>:-fsanitize=address,undefined>)
