// Copyright 2022 Jeffrey E. Trull

#include <stdint.h>

// USB interfaces

// control interface doesn't need a "settings" register (it is defined by standard)
volatile __xdata uint8_t __at(0xfe01) CTL_CTRL;  // control endpoint control register
volatile __xdata uint8_t __at(0xfe02) CTL_FIFO;  // control fifo control register

volatile __xdata uint8_t __at(0xfe03) BLKI_SETR; // bulk input endpoint settings
volatile __xdata uint8_t __at(0xfe04) BLKI_CTRL; // bulk input control register
volatile __xdata uint8_t __at(0xfe05) BLKI_FIFO; // bulk input FIFO (read-only)

volatile __xdata uint8_t __at(0xfe06) BLKO_SETR; // bulk output endpoint settings
volatile __xdata uint8_t __at(0xfe07) BLKO_CTRL; // bulk output control register
volatile __xdata uint8_t __at(0xfe08) BLKO_FIFO; // bulk output FIFO (write-only)

volatile __xdata uint8_t __at(0xfe09) INTR_SETR; // interrupt endpoint settings
volatile __xdata uint8_t __at(0xfe0a) INTR_CTRL; // interrupt control register
volatile __xdata uint8_t __at(0xfe0b) INTR_FIFO; // interrupt FIFO (write-only)

// Fine control over interrupts

// INT0 (USB-related interrupts)
volatile __xdata uint8_t __at(0xfe15) INTENR0;   // interrupt 0 enables by source
volatile __xdata uint8_t __at(0xfe17) INTFLR0;   // interrupt 0 actual sources

// INT1 (not sure what this is, yet)
volatile __xdata uint8_t __at(0xffb5) INTENR1;   // interrupt 1 enables by source

// DMA stuff

volatile __xdata uint8_t __at(0xfe19) DMACTRL;   // there may be more! 0xffe1, 0xffed?

volatile __xdata uint8_t __at(0xfe98) DMA_ADDR0; // transfer address start offset?
volatile __xdata uint8_t __at(0xfe99) DMA_ADDR1;
volatile __xdata uint8_t __at(0xfe9a) DMA_ADDR2;

volatile __xdata uint8_t __at(0xff9e) DMACLR;    // transfer count
volatile __xdata uint8_t __at(0xff9f) DMACMR;    // there may be a high byte (unused)?
