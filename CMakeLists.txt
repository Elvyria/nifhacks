cmake_minimum_required(VERSION 3.10)

if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE
      "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
      CACHE STRING "")
endif()

project(
  nifhacks
  VERSION 0.2
  DESCRIPTION ""
  LANGUAGES CXX)

if(MSVC)
  add_compile_options(/EHsc /bigobj)
endif()

find_package(fmt CONFIG REQUIRED)
find_package(Boost REQUIRED)
find_package(tinyobjloader CONFIG REQUIRED)

add_subdirectory(lib/nif)

add_executable(nifhacks src/main.cpp)

target_compile_features(nifhacks PRIVATE cxx_std_17)

target_link_libraries(nifhacks PRIVATE fmt::fmt-header-only)
target_link_libraries(nifhacks PRIVATE tinyobjloader::tinyobjloader)
target_link_libraries(nifhacks PRIVATE Boost::boost)
target_link_libraries(nifhacks PRIVATE bnos-nif)
