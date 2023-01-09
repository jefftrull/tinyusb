CFLAGS += \
  -DTUP_DCD_ENDPOINT_MAX=8 \
  --model-large \
  --stack-auto \
  --fomit-frame-pointer

LDFLAGS = \
  --code-size 0xe000 \
  --xram-loc 0xe000 \
  --xram-size 0x1000

MCU_DIR = hw/mcu/ali/m56xx

INC += 	$(TOP)/$(MCU_DIR)

SRC += cs4200f.c

# inhibit -Os flag from being added
CFLAGS_OPTIMIZED =
