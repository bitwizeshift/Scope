cmake_minimum_required(VERSION 3.4)

if (PROJECT_NAME)
  set(SCOPE_IS_SUBPROJECT TRUE)
endif ()

##############################################################################
# Project Options
##############################################################################

option(SCOPE_ENABLE_UNIT_TESTS "Compile and run the unit tests for this library" OFF)

##############################################################################
# Project
##############################################################################

if (NOT CMAKE_TESTING_ENABLED AND SCOPE_ENABLE_UNIT_TESTS)
  enable_testing()
endif ()

project(Scope
  VERSION "0.1.0"
  LANGUAGES CXX
)

set(SCOPE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR} CACHE INTERNAL "Major version of the Scope library")
set(SCOPE_VERSION_MINOR ${PROJECT_VERSION_MINOR} CACHE INTERNAL "Minor version of the Scope library")
set(SCOPE_VERSION_PATCH ${PROJECT_VERSION_PATCH} CACHE INTERNAL "Patch version of the Scope library")
set(SCOPE_VERSION ${PROJECT_VERSION} CACHE INTERNAL "Version of Scope library")

# If using conan, only set the find paths. This project is trying to be as
# idiomatic for CMake as possible, and doesn't want to rely on deep hooks from
# a 3rd-party package manager.
if (EXISTS "${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.cmake")
  include("${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.cmake")
  conan_set_find_paths()
endif ()

add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
  INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
  INTERFACE $<INSTALL_INTERFACE:include>
)

target_compile_features(${PROJECT_NAME}
  INTERFACE
    cxx_alias_templates
    cxx_attributes
    cxx_auto_type
    cxx_defaulted_functions
    cxx_defaulted_move_initializers
    cxx_delegating_constructors
    cxx_deleted_functions
    cxx_inheriting_constructors
    cxx_lambdas
    cxx_noexcept
    cxx_right_angle_brackets
    cxx_static_assert
    cxx_rvalue_references
    cxx_uniform_initialization
)

if (SCOPE_ENABLE_UNIT_TESTS)
  add_subdirectory("test")
endif ()

##############################################################################
# Installation
##############################################################################

if (SCOPE_IS_SUBPROJECT)
  return()
endif ()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(SCOPE_CMAKE_CONFIG_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")

# The generated *ConfigVersion is strongly tied to the architecture
# it was generated on, and sets variables such as 'SIZEOF_VOID_P'.
# Since this library is header-only, the host architecture does not
# actually affect the target that consumes this project, so we fake
# the variable to be empty, but reset it after.
#
# Otherwise a 64-bit creator would cause a 32-bit consumer to fail to
# use this library, even though it's header-only.
set(SCOPE_CMAKE_SIZEOF_VOID_P ${CMAKE_SIZEOF_VOID_P})
set(CMAKE_SIZEOF_VOID_P "")
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
  VERSION "${PROJECT_VERSION}"
  COMPATIBILITY "SameMajorVersion"
)
set(CMAKE_SIZEOF_VOID_P ${SCOPE_CMAKE_SIZEOF_VOID_P})

configure_package_config_file(
  "${CMAKE_CURRENT_LIST_DIR}/cmake/${PROJECT_NAME}Config.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
  INSTALL_DESTINATION "${SCOPE_CMAKE_CONFIG_DESTINATION}"
)

# Targets
install(
  TARGETS "${PROJECT_NAME}"
  EXPORT "${PROJECT_NAME}Targets"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)
install(
  EXPORT "${PROJECT_NAME}Targets"
  NAMESPACE "${PROJECT_NAME}::"
  DESTINATION "${SCOPE_CMAKE_CONFIG_DESTINATION}"
)
install(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
  DESTINATION "${SCOPE_CMAKE_CONFIG_DESTINATION}"
)

# Includes
install(
  DIRECTORY "include/"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)
