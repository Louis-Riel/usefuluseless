# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.14

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake3

# The command to remove a file.
RM = /usr/bin/cmake3 -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/nix/Documents/PlatformIO/Projects/usefulUseless

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/nix/Documents/PlatformIO/Projects/usefulUseless/build

# Utility rule file for menuconfig.

# Include the progress variables for this target.
include CMakeFiles/menuconfig.dir/progress.make

CMakeFiles/menuconfig:
	/home/nix/.platformio/penv/bin/python /home/nix/esp/esp-idf/tools/kconfig_new/confgen.py --kconfig /home/nix/esp/esp-idf/Kconfig --sdkconfig-rename /home/nix/esp/esp-idf/sdkconfig.rename --config /home/nix/Documents/PlatformIO/Projects/usefulUseless/sdkconfig --env-file /home/nix/Documents/PlatformIO/Projects/usefulUseless/build/config.env --env IDF_TARGET=esp32 --output config /home/nix/Documents/PlatformIO/Projects/usefulUseless/sdkconfig
	/usr/bin/cmake3 -E env COMPONENT_KCONFIGS_SOURCE_FILE=/home/nix/Documents/PlatformIO/Projects/usefulUseless/build/kconfigs.in COMPONENT_KCONFIGS_PROJBUILD_SOURCE_FILE=/home/nix/Documents/PlatformIO/Projects/usefulUseless/build/kconfigs_projbuild.in IDF_CMAKE=y IDF_TARGET=esp32 KCONFIG_CONFIG=/home/nix/Documents/PlatformIO/Projects/usefulUseless/sdkconfig IDF_TARGET=esp32 MENUCONFIG_STYLE=aquatic /home/nix/.platformio/penv/bin/python /home/nix/esp/esp-idf/tools/kconfig_new/menuconfig.py /home/nix/esp/esp-idf/Kconfig
	/home/nix/.platformio/penv/bin/python /home/nix/esp/esp-idf/tools/kconfig_new/confgen.py --kconfig /home/nix/esp/esp-idf/Kconfig --sdkconfig-rename /home/nix/esp/esp-idf/sdkconfig.rename --config /home/nix/Documents/PlatformIO/Projects/usefulUseless/sdkconfig --env-file /home/nix/Documents/PlatformIO/Projects/usefulUseless/build/config.env --env IDF_TARGET=esp32 --output config /home/nix/Documents/PlatformIO/Projects/usefulUseless/sdkconfig

menuconfig: CMakeFiles/menuconfig
menuconfig: CMakeFiles/menuconfig.dir/build.make

.PHONY : menuconfig

# Rule to build all files generated by this target.
CMakeFiles/menuconfig.dir/build: menuconfig

.PHONY : CMakeFiles/menuconfig.dir/build

CMakeFiles/menuconfig.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/menuconfig.dir/cmake_clean.cmake
.PHONY : CMakeFiles/menuconfig.dir/clean

CMakeFiles/menuconfig.dir/depend:
	cd /home/nix/Documents/PlatformIO/Projects/usefulUseless/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/nix/Documents/PlatformIO/Projects/usefulUseless /home/nix/Documents/PlatformIO/Projects/usefulUseless /home/nix/Documents/PlatformIO/Projects/usefulUseless/build /home/nix/Documents/PlatformIO/Projects/usefulUseless/build /home/nix/Documents/PlatformIO/Projects/usefulUseless/build/CMakeFiles/menuconfig.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/menuconfig.dir/depend

