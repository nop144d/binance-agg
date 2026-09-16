import os

from conan import ConanFile
from conan.tools.build import check_min_cppstd
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout


class BinanceAggConan(ConanFile):
    name = "binance-agg"
    settings = "os", "arch", "compiler", "build_type"

    default_options = {
        # Beast/Asio are header-only; avoids building all of Boost from source.
        "boost/*:header_only": True,
    }

    def requirements(self):
        self.requires("boost/1.91.0")
        self.requires("openssl/3.6.4")
        self.requires("nlohmann_json/3.12.0")
        self.requires("spdlog/1.17.0")

    def build_requirements(self):
        self.test_requires("gtest/1.18.0")

    def validate(self):
        check_min_cppstd(self, 23)

    def layout(self):
        # Per-OS build trees so WSL and Windows can share one checkout:
        #   Linux:   build/linux/<build_type>/generators  (single-config)
        #   Windows: build/windows/generators             (multi-config VS)
        # Must match the paths in CMakePresets.json.
        cmake_layout(self, build_folder=f"build/{str(self.settings.os).lower()}")

        # cmake_layout appends "Release"/"Debug"; lowercase it.
        build_type = str(self.settings.build_type)
        if os.path.basename(self.folders.build) == build_type:
            self.folders.build = os.path.join(
                os.path.dirname(self.folders.build), build_type.lower()
            )
            self.folders.generators = os.path.join(self.folders.build, "generators")

    def generate(self):
        CMakeDeps(self).generate()
        tc = CMakeToolchain(self)
        # CMakePresets.json in the repo is the source of truth for IDEs.
        tc.user_presets_path = False
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
        cmake.ctest(cli_args=["--output-on-failure"])
