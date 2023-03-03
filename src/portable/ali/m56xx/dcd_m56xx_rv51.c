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
 */

// DCD "Device Setup" stuff as described in
// https://github.com/hathach/tinyusb/blob/master/docs/contributing/porting.rst

#include <stdint.h>
#include <stdbool.h>

#include "device/dcd.h"

#include "hw/mcu/ali/m56xx/m5623_rv51.h"

#if CFG_TUD_ENABLED && (CFG_TUSB_MCU == OPT_MCU_M5623_RV51)

void write_mtvec(void (*fn)(void)) {
    __asm__ volatile ("csrw    mtvec, %0"
                      : /* output: none */
                      : "r" ((uint32_t)(fn)) /* input : from register */
                      : /* clobbers: none */);
}

// for debugging
void usb_intr_isr(void);  // in board code cs4200f.c
bool usb_intr_putc(char c);
bool usb_intr_putc_hex(char c);
bool usb_intr_puts(char const * s);

static void __attribute__ ((interrupt ("machine"))) dcd_isr(void)  {
    *(volatile uint8_t*)0xffd0 &= 0xf7;   // light on
//    *(volatile uint8_t*)0xffd0 ^= 0x08;   // toggle light
//    __asm__("ebreak");
    if (int_src & 0x10) {
        // interrupt endpoint transmit done
        usb_intr_isr();
    }

}

void dcd_init       (uint8_t rhport) {
    (void)rhport;

    *(volatile uint8_t*)0xffd0 |= 0x08;   // light off

    // initialize control endpoint
    *CTL_CTRL = 0x10;
    *CTL_CTRL = 0;

    //
    // set up RISCV interrupt handling in the emulator
    //

    // set mtvec to our (consolidated) interrupt handler
    write_mtvec(dcd_isr);

    // set bit 11 (MEIE) of mie to enable external interrupts
    uint32_t mie;
    __asm__ volatile ("csrr  %0, mie" : "=r"(mie));
    mie |= ((uint32_t)1 << 11);
    __asm__ volatile ("csrw mie, %0" : "=r"(mie));

    // set bit 3 (MIE) of mstatus to enable all interrupts
    uint32_t mstatus;
    __asm__ volatile ("csrr  %0, mstatus" : "=r"(mstatus));
    mstatus |= ((uint32_t)1 << 3);
    __asm__ volatile ("csrw mstatus, %0" : "=r"(mstatus));

    //
    // set up 8051 level interrupts
    //

    // enable granular Reset, Rx and Tx interrupts for control endpoint
    // (effectively ANDed with EX0, so this doesn't fully enable them)
    *INTENR0 |= 0x83;
    // enable EX0 (USB) and general interrupts
    *IE |= 0x81;
}


void dcd_int_enable (uint8_t rhport) {}

void dcd_int_disable(uint8_t rhport) {}

// ODDLY this does not get called
// docs say "It will be called by application in the MCU USB interrupt handler"
void dcd_int_handler(uint8_t rhport) {}

void dcd_set_address(uint8_t rhport, uint8_t dev_addr) {}

void dcd_remote_wakeup(uint8_t rhport) {}

void dcd_connect(uint8_t rhport) {}

void dcd_disconnect(uint8_t rhport) {}

// dcd_event_bus_signal and dcd_event_setup_received should be called by our code

// DCD endpoint stuff

bool dcd_edpt_open            (uint8_t rhport, tusb_desc_endpoint_t const * desc_ep) {
    return false;
}

void dcd_edpt_close (uint8_t rhport, uint8_t ep_addr) {}

bool dcd_edpt_xfer (uint8_t rhport, uint8_t ep_addr, uint8_t * buffer, uint16_t total_bytes) {
    return false;   // I guess?
}

// we must call dcd_edpt_xfer_complete

void dcd_edpt_stall (uint8_t rhport, uint8_t ep_addr) {}

void dcd_edpt_clear_stall     (uint8_t rhport, uint8_t ep_addr) {}

#endif  // M5623, the only one for now
