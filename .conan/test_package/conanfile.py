#!/usr/bin/env python
import os
from conans import ConanFile, CMake

class ScopeConanTest(ConanFile):
    settings = "os", "compiler", "arch", "build_type"
    generators = "cmake"

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def test(self):
        if str(self.settings.os) in ["Linux", "Macos"]:
            self.run("Scope.ConanTestPackage")
        elif str(self.settings.os) in ["Windows"]:
            self.run("Scope.ConanTestPackage.exe")
        else:
            self.output.warn("Skipping unit test execution due to cross compiling for {}".format(self.settings.os))
