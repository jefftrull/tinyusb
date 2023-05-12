/* 
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Jeffrey E. Trull
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
 * This file is part of the TinyUSB stack.
 */

#include <stdbool.h>
#include <stdint.h>
#include <mcs51/8052.h>

#include "hw/mcu/ali/m56xx/m5623.h"

volatile uint8_t * main_light_ctl = (uint8_t*)0xffd0;

//
// Timer functionality
//
const uint16_t TIMER2_RELOAD = 12000;  // produces 25ms interrupts
static volatile uint32_t cycles_elapsed = 0;

void timer2_isr (void) __interrupt (5) {
    TF2 = 0;  // reset and reload from RCAP2

    ++cycles_elapsed;
}

void timer_init (void) {
    // set up timer 2

    T2CON = 0;    // stop count and configure as internal timer
    uint16_t reload_with = TIMER2_RELOAD;
    RCAP2H = (reload_with & 0xff00) >> 8;
    RCAP2L = reload_with & 0x00ff;
    TH2 = RCAP2H;
    TL2 = RCAP2L;
    TR2 = 1;     // start counting
    ET2 = 1;     // enable interrupt
}

//--------------------------------------------------------------------+
// Board porting API
//--------------------------------------------------------------------+

void board_led_write(bool state) {
    if (state) {
        *main_light_ctl &= 0xf7;
    }
    else
        *main_light_ctl |= 0x08;
}

uint32_t board_millis(void) {
    // atomic read of current cycle count
    ET2 = 0;
    uint32_t cycles = cycles_elapsed;
    ET2 = 1;

    return 25 * cycles;
}

void board_init(void) {
    // turn off light so we know we got this far
    *main_light_ctl |= 0x08;

    // turn off unwanted interrupts
    ET0 = 0;
    ET1 = 0;
    EX0 = 0;
    EX1 = 0;

    // configure endpoints. see e.g. EPCSETR in 5621 docs
    BLKO_SETR = 0x90;   // bulk output endpoint 2
    BLKI_SETR = 0x88;   // bulk input endpoint 1
    INTR_SETR = 0xd9;   // interrupt endpoint 3


    timer_init();

    EA = 1;      // enable interrupts overall
}

// we don't have these, so it's a little alarming they seem to be required
int board_uart_read(uint8_t* buf, int len) { return 0; }
int board_uart_write(void const * buf, int len) { return 0; }

void do_breakpoint() {
    // get the PC where we were called
    // PC+3 will be stored on the stack
    __idata const uint8_t * TOS = (__idata const uint8_t *)SP;
    uint16_t brk_pc = (uint16_t)(((*TOS) << 8) | *(TOS-1)) - 3;
//    usb_intr_puts("BRK="); usb_intr_putc_hex(brk_pc >> 8); usb_intr_putc_hex(brk_pc & 0xff); usb_intr_putc('\n');

    while (1)
        *main_light_ctl ^= 0x08;
}
