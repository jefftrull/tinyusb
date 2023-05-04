CFLAGS += \
  -DBOARD_TUD_MAX_SPEED=OPT_MODE_HIGH_SPEED \
  -DTUP_DCD_ENDPOINT_MAX=8 \
  -march=rv32im -mabi=ilp32 \
  -nostdlib \
  -nostartfiles \
  -nodefaultlibs \
  -fno-builtin \
  -ffreestanding \
  -Wno-unused-parameter

CROSS_COMPILE = riscv32-unknown-elf-

ASFLAGS = -march=rv32im

MCU_DIR = hw/mcu/ali/m56xx

SRC_C += src/portable/ali/m56xx/dcd_m56xx_rv51.c

INC +=  $(TOP)/$(MCU_DIR)
