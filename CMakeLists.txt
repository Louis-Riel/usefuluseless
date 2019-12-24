cmake_minimum_required(VERSION 3.5)
set(ENV{IOT_SOLUTION_PATH} "/home/nix/Documents/PlatformIO/Projects/esp-iot-solution")

# include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Overwrite the IDF_PATH to the esp-idf path in submodule.
set(ENV{IDF_PATH} "$ENV{IOT_SOLUTION_PATH}/submodule/esp-idf/")

include($ENV{IOT_SOLUTION_PATH}/components/CMakeLists.txt)

project(spi_master)
