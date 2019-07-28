find_package(Catch2 REQUIRED)

##############################################################################
# Target
##############################################################################

set(source_files
  src/main.cpp
  src/scope/scope_exit.test.cpp
  src/scope/scope_fail.test.cpp
  src/scope/scope_success.test.cpp
  src/scope/unique_resource.test.cpp
)

add_executable(${PROJECT_NAME}Test
  ${source_files}
)
add_executable(${PROJECT_NAME}::Test ALIAS ${PROJECT_NAME}Test)

if ( "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" OR
     "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" )
  target_compile_options(${PROJECT_NAME}Test
    PRIVATE -Wall -Wextra -Werror -pedantic
  )
elseif( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC" )
  # TODO: Determine MSVC necessary compiler flags
endif ()

set_target_properties(${PROJECT_NAME}Test
  PROPERTIES
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_link_libraries(${PROJECT_NAME}Test
  PRIVATE Catch2::Catch2
  PRIVATE ${PROJECT_NAME}::${PROJECT_NAME}
)

##############################################################################
# CTest
##############################################################################

include(Catch)
catch_discover_tests(${PROJECT_NAME}Test
  WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}"
)