set(STM_MCU_FAMILY STM32 CACHE INTERNAL "STM MCU family")
set(STM32_MCU_SERIE STM32U5 CACHE INTERNAL "STM32 MCU serie")
set(CPU_ARCH "ARMv8-M" CACHE INTERNAL "CPU architecture for micrium.")

set(CMAKE_CXX_FLAGS
    "-mcpu=cortex-m33 -mthumb -mfpu=fpv5-sp-d16 -mfloat-abi=hard -fmessage-length=0 -ffunction-sections -fdata-sections -fno-common -fsingle-precision-constant -Wdouble-promotion -fno-move-loop-invariants -fno-stack-protector -Wall -Wextra -Wshadow=compatible-local"
    CACHE STRING
    "Flags used by the C++ compiler during all build types."
    FORCE
)

# NOT "${CMAKE_C_FLAGS} ..." -- this is a CACHE ... FORCE, so referring to its own current
# value appends another full copy of these flags on every re-configure, and CMake
# re-configures whenever any CMakeLists.txt changes. The cached string grows without bound:
# measured at 25 copies and 7522 characters in build directories that had been reconfigured
# often, at which point the LINK command line exceeded the Windows limit and the build failed
# with "The command line is too long" -- a message that points at the linker and says nothing
# about where the length came from. The CXX and ASM flags below never had the self-reference,
# which is what made the asymmetry visible.
set(CMAKE_C_FLAGS
    "-mcpu=cortex-m33 -mthumb -mfpu=fpv5-sp-d16 -mfloat-abi=hard  -fmessage-length=0 -ffunction-sections -fdata-sections -fno-common -fsingle-precision-constant -Wdouble-promotion -fno-move-loop-invariants -fno-stack-protector -Wall -Wextra -Wshadow=compatible-local -Werror=implicit-function-declaration"
    CACHE STRING
    "Flags used by the C compiler during all build types."
    FORCE
)

set(CMAKE_ASM_FLAGS
    "-mcpu=cortex-m33 -mthumb -mfpu=fpv5-sp-d16 -mfloat-abi=hard -fmessage-length=0 -ffunction-sections -fdata-sections -fno-move-loop-invariants -fno-stack-protector -Wall -Wextra -Wshadow=compatible-local"
    CACHE STRING
    "Flags used by the C assembler during all build types."
    FORCE
)

# Debug
set(CMAKE_C_FLAGS_DEBUG "-g3 -ffreestanding -DDEBUGGING" CACHE INTERNAL "c compiler flags debug")

set(CMAKE_CXX_FLAGS_DEBUG "-g3 -ffreestanding -fno-exceptions -fno-rtti -fno-use-cxa-atexit -fno-threadsafe-statics -DDEBUG" CACHE INTERNAL "cxx compiler flags debug")

set(CMAKE_ASM_FLAGS_DEBUG "-g3" CACHE INTERNAL "asm compiler flags debug")

set(CMAKE_EXE_LINKER_FLAGS
    "-specs=nosys.specs -specs=nano.specs -u _printf_float -Xlinker -Map=output.map -Wl,--gc-sections -Wl,--print-memory-usage -fno-exceptions -fno-rtti -lm -T${STM32U5_LINKER_SCRIPT_PATH}"
    CACHE STRING
    "Flags used by the linker during all build types."
    FORCE
)
