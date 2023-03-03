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

#include "src/common/tusb_common.h"
#include "hw/mcu/ali/m56xx/m5623_rv51.h"

void board_init(void) {
    *IE &= ~((uint8_t)0x81);   // turn off EA and EX0

    // set up interrupt endpoint for logging
    *INTR_CTRL = 0x10;   // initialize fifo
    *INTR_CTRL = 0;

    *INTR_CTRL |= 0x02;   // put interrupt endpoint in send mode
    *INTENR0 = 0x10;     // enable "Tx done" for interrupt endpoint

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

//
// Debugging via the interrupt endpoint (in lieu of a UART)
//

// Here we create a small buffer and feed it to the interrupt endpoint,
// one byte at a time, as it completes transmitting the previous byte.

#define INT_BUFFER_SIZE 128
char usb_interrupt_buffer[INT_BUFFER_SIZE];
// rd_ptr == wr_ptr -> empty buffer
// buffer extends from rd_ptr to wr_ptr, always
// we check if we are about to overrun and return false in that case
uint8_t rd_ptr = 0;
uint8_t wr_ptr = 0;

// an interrupt handler for the interrupt endpoint
// (these are two different kinds of interrupts)
// the fourth endpoint is called an "interrupt" endpoint, as opposed
// to control or bulk
// here we handle the periodic polling the host will do for us
// which gets called a USB "interrupt" but is not exactly
// nevertheless we get an 8051 EX0 interrupt with INTFLR0 bit 4 set
void usb_intr_isr(void) {
    if ((rd_ptr != wr_ptr) && (*INTR_CTRL & (uint8_t)(1 << 7))) {
        // there is some data to be sent, and the fifo is empty
        *INTR_FIFO = usb_interrupt_buffer[rd_ptr];
        *INTR_CTRL |= 0x20;  // force send
        // update pointer
        rd_ptr = (rd_ptr + 1) % INT_BUFFER_SIZE;
    }
}

bool usb_intr_putc_lockless(char c) {
    uint8_t next_wr_ptr = (wr_ptr + 1) % INT_BUFFER_SIZE;
    if (next_wr_ptr == rd_ptr)
        return false;  // we would overrun if we accepted this char
    usb_interrupt_buffer[wr_ptr] = c;
    wr_ptr = next_wr_ptr;

    usb_intr_isr();   // will send immediately if it can

    return true;
}

bool usb_intr_putc(char c) {
    *INTENR0 &= ~((uint8_t)0x10);  // atomic update w.r.t. int endpoint

    bool success = usb_intr_putc_lockless(c);

    *INTENR0 |= (uint8_t)0x10;  // release

    return success;
}

bool usb_intr_puts(char const * s) {
    *INTENR0 &= ~((uint8_t)0x10);

    while (*s != '\0') {
        if (!usb_intr_putc_lockless(*s++)) {
            *INTENR0 |= (uint8_t)0x10;  // release
            return false;
        }
    }

    *INTENR0 |= (uint8_t)0x10;  // release

    return true;

}

bool usb_intr_putc_hex(uint8_t c) {
    *INTENR0 &= ~((uint8_t)0x10);  // atomic update w.r.t. int endpoint

    bool success;

    uint8_t msb = (c >> 4);
    if (msb < 10) {
        success = usb_intr_putc_lockless(msb + 0x30);   // 0-9
    } else {
        success = usb_intr_putc_lockless(msb + 0x37);   // A-F
    }

    if (!success) {
        *INTENR0 |= (uint8_t)0x10;  // release
        return false;
    }

    uint8_t lsb = c & 0x0f;
    if (lsb < 10) {
        success = usb_intr_putc_lockless(lsb + 0x30);   // 0-9
    } else {
        success = usb_intr_putc_lockless(lsb + 0x37);   // A-F
    }

    *INTENR0 |= (uint8_t)0x10;  // release

    return success;
}

// output uint16_t LSB first a la Wireshark
bool usb_intr_put_hex4(uint16_t val) {
    if (!usb_intr_putc_hex(val & 0xff))
        return false;

    if (!usb_intr_putc(' '))
        return false;

    if (!usb_intr_putc_hex(val >> 8))
        return false;

    if (!usb_intr_putc(' '))
        return false;

    return true;
}

bool usb_intr_put_req(tusb_control_request_t const * req) {
    if (!usb_intr_puts("REQ "))
        return false;

    if (!usb_intr_putc_hex(req->bmRequestType))
        return false;

    if (!usb_intr_putc(' '))
        return false;

    if (!usb_intr_putc_hex(req->bRequest))
        return false;

    if (!usb_intr_putc(' '))
        return false;

    // show wValue, wIndex, wLength  in little endian order, matching Wireshark
    if (!usb_intr_put_hex4(req->wValue))
        return false;

    if (!usb_intr_put_hex4(req->wIndex))
        return false;

    if (!usb_intr_put_hex4(req->wLength))
        return false;

    return true;

}

