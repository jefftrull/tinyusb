include ../../../tools/top.mk
include ../../make.mk

INC += \
	src \
	$(TOP)/hw \

CFLAGS += -DCFG_EXAMPLE_MSC_READONLY

# Example source
EXAMPLE_SOURCE += $(wildcard src/*.c)
SRC_C += $(addprefix $(CURRENT_PATH)/, $(EXAMPLE_SOURCE))

include ../../rules.sdcc
