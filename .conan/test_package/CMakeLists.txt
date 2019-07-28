cmake_minimum_required(VERSION 3.8)

project(Scope.ConanTestPackage)

include("${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
conan_set_find_paths()

find_package(Scope REQUIRED)

set(source_files
  src/main.cpp
)

add_executable(${PROJECT_NAME}
  ${source_files}
)
target_link_libraries(${PROJECT_NAME}
  PRIVATE Scope::Scope
)