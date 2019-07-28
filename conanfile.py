#!/usr/bin/env python3

from conans import ConanFile, CMake

class ScopeConan(ConanFile):

    # Package Info
    name = "Scope"
    version = "0.1.0"
    description = "An RAII scope-guard library"
    url = "https://github.com/bitwizeshift/Scope"
    author = "Matthew Rodusek <matthew.rodusek@gmail.com>"
    license = "BSL-1.0"

    # Sources
    exports = ( "LICENSE" )
    exports_sources = ( "CMakeLists.txt",
                        "cmake/*",
                        "include/*",
                        "LICENSE" )

    # Settings
    options = {}
    default_options = {}
    generators = "cmake"

    # Dependencies
    build_requires = ("Catch2/2.7.1@catchorg/stable")


    def configure_cmake(self):
        cmake = CMake(self)

        # Features
        cmake.definitions["SCOPE_ENABLE_UNIT_TESTS"] = "OFF"

        cmake.configure()
        return cmake


    def build(self):
        cmake = self.configure_cmake()
        cmake.build()
        # cmake.test()


    def package(self):
        cmake = self.configure_cmake()
        cmake.install()

        self.copy(pattern="LICENSE", dst="licenses")


    def package_id(self):
        self.info.header_only()
