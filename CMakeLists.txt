cmake_minimum_required(VERSION 3.10)

if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE
      "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
      CACHE STRING "")
endif()

project(
  nifhacks
  VERSION 0.1
  DESCRIPTION ""
  LANGUAGES CXX)

if(MSVC)
  add_definitions("/bigobj")
  add_definitions("/EHsc")
endif()

find_package(fmt CONFIG REQUIRED)
find_package(Boost REQUIRED COMPONENTS filesystem)
find_path(FPLUS_INCLUDE_DIRS "fplus/benchmark_session.hpp")
find_package(tinyobjloader CONFIG REQUIRED)

add_subdirectory(lib/nif)

add_executable(nifhack src/main.cpp)

target_link_libraries(nifhack PRIVATE fmt::fmt)
target_link_libraries(nifhack PRIVATE Boost::boost Boost::filesystem)
target_link_libraries(nifhack PRIVATE tinyobjloader::tinyobjloader)
target_include_directories(nifhack PRIVATE ${FPLUS_INCLUDE_DIRS})

target_link_libraries(nifhack PUBLIC bnos-nif)
