# CMakeList.txt : CMake project for 3D engine, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_executable (CMakeTarget "3D engine.cpp" "3D engine.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET CMakeTarget PROPERTY CXX_STANDARD 20)
endif()

project(My3DEngine)

# Find the libraries
find_package(SDL2 REQUIRED)
find_package(glad REQUIRED)
find_package(glm REQUIRED)

# Source file
add_executable(My3DEngine "src/3D engine.cpp")

# Link everything together
target_link_libraries(My3DEngine PRIVATE SDL2::SDL2 glad::glad)
target_include_directories(My3DEngine PRIVATE ${GLM_INCLUDE_DIRS})

#sub-libraries
add_subdirectory(My3DEngine)
