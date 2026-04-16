# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.10)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project(My3DEngine)

# Find the libraries
find_package(SDL2 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

# Source file
add_executable(My3DEngine "D:/Blbosti XD/c+ cvičení/3D engine/3D engine/src/3Dengine.cpp")

# Link everything together
target_link_libraries(My3DEngine PRIVATE SDL2::SDL2 SDL2::SDL2main glad::glad)
target_include_directories(My3DEngine PRIVATE ${GLM_INCLUDE_DIRS})