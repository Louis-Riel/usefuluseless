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
CMAKE_SOURCE_DIR = /home/nix/esp/esp-idf/components/bootloader/subproject

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/nix/Documents/PlatformIO/Projects/usefulUseless/build/bootloader

# Utility rule file for size-files.

# Include the progress variables for this target.
include CMakeFiles/size-files.dir/progress.make

CMakeFiles/size-files: bootloader.elf
	/home/nix/.platformio/penv/bin/python /home/nix/esp/esp-idf/tools/idf_size.py --files /home/nix/Documents/PlatformIO/Projects/usefulUseless/build/bootloader/bootloader.map

size-files: CMakeFiles/size-files
size-files: CMakeFiles/size-files.dir/build.make

.PHONY : size-files

# Rule to build all files generated by this target.
CMakeFiles/size-files.dir/build: size-files

.PHONY : CMakeFiles/size-files.dir/build

CMakeFiles/size-files.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/size-files.dir/cmake_clean.cmake
.PHONY : CMakeFiles/size-files.dir/clean

CMakeFiles/size-files.dir/depend:
	cd /home/nix/Documents/PlatformIO/Projects/usefulUseless/build/bootloader && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/nix/esp/esp-idf/components/bootloader/subproject /home/nix/esp/esp-idf/components/bootloader/subproject /home/nix/Documents/PlatformIO/Projects/usefulUseless/build/bootloader /home/nix/Documents/PlatformIO/Projects/usefulUseless/build/bootloader /home/nix/Documents/PlatformIO/Projects/usefulUseless/build/bootloader/CMakeFiles/size-files.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/size-files.dir/depend

