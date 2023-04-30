// Copyright 2022 Jeffrey E. Trull

#include <stdint.h>

// MCU SFRs
static volatile uint8_t * const IE = (volatile uint8_t *)0xc00000a8;

// USB interfaces

// control interface doesn't need a "settings" register (it is defined by standard)
static volatile uint8_t * const CTL_CTRL = (uint8_t *)0xfe01;  // control endpoint control register
static volatile uint8_t * const CTL_FIFO = (uint8_t *)0xfe02;  // control fifo control register

static volatile uint8_t * const BLKI_SETR = (uint8_t *)0xfe03; // bulk input endpoint settings
static volatile uint8_t * const BLKI_CTRL = (uint8_t *)0xfe04; // bulk input control register
static volatile uint8_t * const BLKI_FIFO = (uint8_t *)0xfe05; // bulk input FIFO (write-only)

static volatile uint8_t * const BLKO_SETR = (uint8_t *)0xfe06; // bulk output endpoint settings
static volatile uint8_t * const BLKO_CTRL = (uint8_t *)0xfe07; // bulk output control register
static volatile const uint8_t * const BLKO_FIFO = (uint8_t *)0xfe08; // bulk output FIFO (read-only)

static volatile uint8_t * const INTR_SETR = (uint8_t *)0xfe09; // interrupt endpoint settings
static volatile uint8_t * const INTR_CTRL = (uint8_t *)0xfe0a; // interrupt control register
static volatile uint8_t * const INTR_FIFO = (uint8_t *)0xfe0b; // interrupt FIFO (write-only)

// Fine control over interrupts

// INT0 (USB-related interrupts)
static volatile uint8_t * const INTENR0 = (uint8_t *)0xfe15;   // interrupt 0 enables by source
static volatile uint8_t * const INTFLR0 = (uint8_t *)0xfe17;   // interrupt 0 actual sources

// INT1 (not sure what this is, yet)
static volatile uint8_t * const INTENR1 = (uint8_t *)0xffb5;   // interrupt 1 enables by source

// DMA stuff

static volatile uint8_t * const DMACTL = (uint8_t *)0xfe19;    // there may be more! 0xffe1, 0xffed?

static volatile uint8_t * const DMA_ADDR0 = (uint8_t *)0xfe98; // transfer address start offset?
static volatile uint8_t * const DMA_ADDR1 = (uint8_t *)0xfe99;
static volatile uint8_t * const DMA_ADDR2 = (uint8_t *)0xfe9a;

static volatile uint8_t * const DMACLR = (uint8_t *)0xff9e;    // transfer count
static volatile uint8_t * const DMACMR = (uint8_t *)0xff9f;    // there may be a high byte (unused)?
