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

#include "hw/mcu/ali/m56xx/m5623_rv51.h"

void board_init(void) {
    *IE &= ~((uint8_t)0x81);   // turn off EA and EX0

    // set up interrupt endpoint for logging
    *INTR_CTRL = 0x10;   // initialize fifo
    *INTR_CTRL = 0;

    *INTR_CTRL |= 0x02;   // put interrupt endpoint in send mode

    *IE |= 0x81;         // enable EA/EX0
}

volatile uint8_t * main_light_ctl = (uint8_t*)0xffd0;
void board_led_write(bool state) {
    if (state) {
        *main_light_ctl &= 0xf7;
    }
    else
        *main_light_ctl |= 0x08;
}

#define csr_read(csr)                               \
({                                                  \
    register uint32_t v;                            \
    __asm__ __volatile__ ("csrr %0, %1"             \
                  : "=r" (v)                        \
                  : "n" (csr)                       \
                  : "memory");                      \
    v;                                              \
})

uint32_t board_millis(void) {
    // counter seems to roll over roughly every 25ms with the 12000 reload
    return 25 * (csr_read(0xb00) >> 16);
}

// needed for CDC. We will come up with something.
int board_uart_read(uint8_t* buf, int len) { return 0; }
int board_uart_write(void const * buf, int len) { return 0; }
