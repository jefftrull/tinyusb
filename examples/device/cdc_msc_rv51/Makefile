include ../../../tools/top.mk
include ../../make.mk

INC += \
	src \
	$(TOP)/hw \

# Example source
EXAMPLE_SOURCE += $(wildcard src/*.c)
SRC_C += $(addprefix $(CURRENT_PATH)/, $(EXAMPLE_SOURCE))

# startup stuff for the emulated RISCV
SRC_S += $(addprefix $(CURRENT_PATH)/, src/init.S)

LDFLAGS += -Wl,-T linker.ld -Wl,-Map hello-rv32.map

OBJCOPY = riscv32-unknown-elf-objcopy

include ../../rules.mk

