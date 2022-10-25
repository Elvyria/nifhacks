cmake_minimum_required(VERSION 3.10)

project(nifhacks LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

add_subdirectory(lib/nif)

add_executable(nifhacks src/main.cpp)
set_target_properties(nifhacks PROPERTIES DEBUG_POSTFIX "${CMAKE_DEBUG_POSTFIX}")

target_link_libraries(nifhacks bnos-nif)
target_link_libraries(nifhacks ${CONAN_LIBS})
