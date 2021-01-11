add_library(
  bnos-nif
  Animation.cpp
  BasicTypes.cpp
  ExtraData.cpp
  Factory.cpp
  Geometry.cpp
  NifFile.cpp
  Nodes.cpp
  Objects.cpp
  Particles.cpp
  Shaders.cpp
  Skin.cpp
  bhk.cpp
  utils/Object3d.cpp)

target_include_directories(bnos-nif PRIVATE ${FPLUS_INCLUDE_DIRS})
target_include_directories(bnos-nif PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}"
                                           "${CMAKE_CURRENT_SOURCE_DIR}/utils")
