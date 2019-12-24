file(REMOVE_RECURSE
  "config/sdkconfig.h"
  "config/sdkconfig.cmake"
  "bootloader/bootloader.elf"
  "bootloader/bootloader.bin"
  "bootloader/bootloader.map"
  "spi_master.bin"
  "spi_master.map"
  "project_elf_src.c"
  "project_elf_src.c"
  "CMakeFiles/spi_master.elf.dir/project_elf_src.c.obj"
  "spi_master.elf.pdb"
  "spi_master.elf"
)

# Per-language clean rules from dependency scanning.
foreach(lang C)
  include(CMakeFiles/spi_master.elf.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
