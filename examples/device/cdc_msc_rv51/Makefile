include ../../../tools/top.mk
include ../../make.mk

INC += \
	src \
	$(TOP)/hw \

CFLAGS += -DCFG_EXAMPLE_MSC_READONLY

# Example source
EXAMPLE_SOURCE += $(wildcard src/*.c)
SRC_C += $(addprefix $(CURRENT_PATH)/, $(EXAMPLE_SOURCE))

# startup stuff for the emulated RISCV
SRC_S += $(addprefix $(CURRENT_PATH)/, src/init.S)

LDFLAGS += -Wl,-T linker.ld -Wl,-Map cdc_msc_rv51.map

OBJCOPY = riscv32-unknown-elf-objcopy

include ../../rules.mk

