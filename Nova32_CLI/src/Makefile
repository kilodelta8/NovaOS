# ============================================================================
# Project NOVA-32 | Phase 1: Bare-Metal CLI Operating System (NovaOS v0.1)
# Target MCU: STMicroelectronics STM32H743ZIT6 (ARM Cortex-M7 @ 480 MHz)
# ============================================================================

TARGET      := novaos_v01
BUILD_DIR   := build

# Cross-Compiler Toolchain
PREFIX      := arm-none-eabi-
CC          := $(PREFIX)gcc
AS          := $(PREFIX)gcc -x assembler-with-cpp
LD          := $(PREFIX)gcc
OBJCOPY     := $(PREFIX)objcopy
OBJDUMP     := $(PREFIX)objdump
SIZE        := $(PREFIX)size
GDB         := $(PREFIX)gdb

# Source Files & Linker Script
SRCS        := startup.c \
               hal_uart.c \
               kernel.c

LDSCRIPT    := nova32.ld

# Object & Dependency Files
OBJS        := $(SRCS:\%.c=$(BUILD_DIR)/%.o)
DEPS        := $(OBJS:.o=.d)

# ============================================================================
# Architecture & Compiler Flags
# ============================================================================
# -mcpu=cortex-m7       : Target ARMv7E-M Cortex-M7 core
# -mthumb               : Generate Thumb-2 instruction set
# -mfpu=fpv5-d16        : Enable hardware double-precision FPU (CP10/CP11)
# -mfloat-abi=hard      : Pass floating-point arguments directly in FPU registers
ARCH_FLAGS  := -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard

CFLAGS      := $(ARCH_FLAGS) \
               -std=c11 \
               -O2 \
               -g3 \
               -Wall \
               -Wextra \
               -Wshadow \
               -Wundef \
               -ffreestanding \
               -fno-builtin \
               -nostdlib \
               -ffunction-sections \
               -fdata-sections \
               -MMD -MP

# ============================================================================
# Linker Flags
# ============================================================================
LDFLAGS     := $(ARCH_FLAGS) \
               -T $(LDSCRIPT) \
               -nostartfiles \
               -nostdlib \
               -Wl,--gc-sections \
               -Wl,--print-memory-usage \
               -Wl,-Map=$(BUILD_DIR)/$(TARGET).map \
               -lgcc

# ============================================================================
# Build Targets
# ============================================================================
.PHONY: all clean flash openocd disasm debug

all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).bin $(BUILD_DIR)/$(TARGET).hex

# Create build directory
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# Compile C source files into object files
$(BUILD_DIR)/\%.o: \%.c \vert{}$(BUILD_DIR)
	@echo "[CC]      $<"
	@$(CC)$(CFLAGS) -c $< -o$@

# Link object files into ELF binary
$(BUILD_DIR)/$(TARGET).elf: $(OBJS)$(LDSCRIPT)
	@echo "[LD]      $@"
	@$(LD)$(OBJS) $(LDFLAGS) -o$@
	@echo "------------------------------------------------------------"
	@$(SIZE) --format=berkeley$@
	@echo "------------------------------------------------------------"

# Extract raw binary image for flashing to 0x08000000
$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	@echo "[OBJCOPY] $@"
	@$(OBJCOPY) -O binary $<$@

# Extract Intel HEX image
$(BUILD_DIR)/$(TARGET).hex: $(BUILD_DIR)/$(TARGET).elf
	@echo "[OBJCOPY] $@"
	@$(OBJCOPY) -O ihex $<$@

# ============================================================================
# Hardware Flashing


# Launch Renode with GUI UART window
emulate: $(BUILD_DIR)/$(TARGET).elf
	renode nova32_phase1.resc

# Launch Renode in pure headless terminal mode (connect via telnet localhost 3456)
emulate-headless: $(BUILD_DIR)/$(TARGET).elf
	renode --disable-xwt --console nova32_phase1.resc