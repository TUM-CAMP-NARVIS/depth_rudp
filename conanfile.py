
from conans import ConanFile, tools, CMake

class TrvlConan(ConanFile):

    name = "trvl"
    version = "0.1.0"
    generators = "cmake"
    settings = "os", "compiler", "build_type", "arch"
    description = "depth compression library"

    exports_sources = "examples/*", "include/*", "src/*", "CMakeLists.txt"

    def requirements(self):
        self.requires("enet/1.3.14@camposs/stable")
        self.requires("opencv/3.4.8@camposs/stable")

    def configure(self):
        if self.settings.os == "Linux":
            self.options["opencv"].with_gtk = True
            self.options["opencv"].shared = True

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def imports(self):
        self.copy(src="bin", pattern="*.dll", dst="./bin") # Copies all dll files from packages bin folder to my "bin" folder
        self.copy(src="lib", pattern="*.dll", dst="./bin") # Copies all dll files from packages bin folder to my "bin" folder
        self.copy(src="lib", pattern="*.dylib*", dst="./lib") # Copies all dylib files from packages lib folder to my "lib" folder
        self.copy(src="lib", pattern="*.so*", dst="./lib") # Copies all so files from packages lib folder to my "lib" folder
        self.copy(src="bin", pattern="ut*", dst="./bin") # Copies all applications
        self.copy(src="bin", pattern="log4cpp.conf", dst="./") # copy a logging config template
