/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Jeffrey E. Trull
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <stdint.h>

// USB interfaces

// control interface doesn't need a "settings" register (it is defined by standard)
volatile __xdata uint8_t __at(0xfe01) CTL_CTRL;  // control endpoint control register
volatile __xdata uint8_t __at(0xfe02) CTL_FIFO;  // control fifo control register

volatile __xdata uint8_t __at(0xfe03) BLKI_SETR; // bulk input endpoint settings
volatile __xdata uint8_t __at(0xfe04) BLKI_CTRL; // bulk input control register
volatile __xdata uint8_t __at(0xfe05) BLKI_FIFO; // bulk input FIFO (write-only)

volatile __xdata uint8_t __at(0xfe06) BLKO_SETR; // bulk output endpoint settings
volatile __xdata uint8_t __at(0xfe07) BLKO_CTRL; // bulk output control register
volatile __xdata const uint8_t __at(0xfe08) BLKO_FIFO; // bulk output FIFO (read-only)

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

volatile __xdata uint8_t __at(0xfe19) DMACTR;    // there may be more! 0xffe1, 0xffed?

volatile __xdata uint8_t __at(0xff98) DMA_ADDR0; // transfer address start offset?
volatile __xdata uint8_t __at(0xff99) DMA_ADDR1;
volatile __xdata uint8_t __at(0xff9a) DMA_ADDR2;

volatile __xdata uint8_t __at(0xff9e) DMACLR;    // transfer count
volatile __xdata uint8_t __at(0xff9f) DMACMR;    // there may be a high byte (unused)?
