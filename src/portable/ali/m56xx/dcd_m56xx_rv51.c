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

// Tx bookkeeping
static uint16_t bytes_sent = 0;       // for remembering how many bytes we put in the fifo
static uint8_t const * remaining_data = NULL;
static uint16_t bytes_remaining = 0;  // data count not yet placed in Tx fifo

// Rx bookkeeping
static uint8_t bytes_requested = 0;
static uint8_t * tusb_rcv_buffer = NULL;

// for isr when there is no pending rcv request
static uint16_t bytes_stored = 0;
static uint8_t rx_buffer[64];

// for debugging
void usb_intr_isr(void);  // in board code cs4200f.c
bool usb_intr_putc(char c);
bool usb_intr_putc_hex(char c);
bool usb_intr_puts(char const * s);
bool usb_intr_put_req(tusb_control_request_t const * req);

uint8_t fill_xmit_fifo(uint8_t const * buffer, uint16_t buffer_size) {
    uint16_t send_count = 0;

    while ((send_count < buffer_size) && (~(*CTL_CTRL) & (1 << 6))) {
        // FIFO not full and we still have data
        *CTL_FIFO = *buffer++;
        ++send_count;
    }

    return send_count;
}

static void __attribute__ ((interrupt ("machine"))) dcd_isr(void);
void dcd_init       (uint8_t rhport) {
    (void)rhport;

    *(volatile uint8_t*)0xffd0 |= 0x08;   // light off

    // initialize control endpoint
    *CTL_CTRL = 0x10;
    *CTL_CTRL = 0;

    // intialize bulk endpoints
    *BLKO_CTRL = 0x10;    // OUT (toward us)
    *BLKO_CTRL = 0;

    *BLKI_CTRL = 0x10;    // IN (toward host)
    *BLKI_CTRL = 0;

    // I have to do this to DMACTL for there to be an Inquiry LUN response
    *DMACTL &= 0xfb;   // bit 2 must be 0, which means "DMA direction out" on the 5621. Strange.

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
    *INTENR0 |= 0x0c;   // also enable bulk interrupts
    // enable EX0 (USB) and general interrupts
    dcd_int_enable(rhport);
}


void dcd_int_enable (uint8_t rhport) {
    *IE |= 0x01;    // EX0 = 1;
}

void dcd_int_disable(uint8_t rhport) {
    *IE &= 0xfe;   // EX0 = 0;
}

static bool first_tx = true;          // discard the first tx complete after firmware update

static void __attribute__ ((interrupt ("machine"))) dcd_isr(void)  {

    *IE &=0xfe;   // EX0 = 0. To my surprise, this is not automatic - JET

    // find out which
    uint8_t int_src = *INTFLR0;  // cleared upon read

    uint32_t cause;
    __asm__ volatile ("csrr %0, mcause" : "=r"(cause));   // read to re-enable interrupt
    (void)cause;                                          // discard

    if (int_src & 0x10) {
        // interrupt endpoint transmit done
        usb_intr_isr();
    }

    if (int_src & 0x01) {
        // Tx done on control endpoint

        if (!first_tx && (bytes_remaining != 0)) {
            // send another chunk

            bool short_transmission = (bytes_remaining < 64);

            uint8_t packet_size = fill_xmit_fifo(remaining_data, bytes_remaining);
            bytes_remaining -= packet_size;
            remaining_data += packet_size;
            bytes_sent += packet_size;
            if (short_transmission) {
                *CTL_CTRL |= (uint8_t)(1 << 5);  // force transmit
            }

        } else if (!first_tx && (bytes_remaining == 0)) {
            // all data en route to host; inform TUSB (which may give us more)
            dcd_event_xfer_complete(0, 0x80, bytes_sent, XFER_RESULT_SUCCESS, true);
        } else {
            // this always happens right after firmware deployment because
            // the previous firmware transmitted a response to the host ("update successful")
            first_tx = false;
        }
    }

    if (int_src & 0x02) {
        *CTL_CTRL &= 0xfd;   // put ctl interface in receive mode

        // Rx done on control endpoint
        if (*CTL_CTRL & 0x04) {
            // this is a setup packet
            uint8_t i = 0;
            uint8_t setup[8];
            while ((i < 8) && (~(*CTL_CTRL) & 0x80)) {
                // FIFO not empty, add another byte
                setup[i++] = *CTL_FIFO;
            }
            dcd_event_setup_received(0, setup, true);

            // Note:
            // M5621 docs list 8 methods implemented in hardware. Presumably M5623 is similar:
            // SET/CLEAR_FEATURE
            // SET/GET_CONFIGURATION
            // SET/GET_INTERFACE
            // SET_ADDRESS
            // GET_STATUS
            // of those SET_ADDRESS and SET_CONFIGURATION are definitely implemented in hardware,
            // and we - experimentally - don't get an interrupt for them

        } else if ((~(*CTL_CTRL) & 0x80)) {
            // we've received something, but it's not a setup packet

            // is there is a pending request from the main thread?
            if (bytes_requested != 0) {
                uint16_t bytes_received = 0;   // for remembering how many bytes we took from the fifo
                while ((bytes_received < bytes_requested) && (~(*CTL_CTRL) & (1 << 7))) {
                    // FIFO not empty and there is space remaining
                    *tusb_rcv_buffer++ = *CTL_FIFO;
                    ++bytes_received;
                }
                // clear pending request
                bytes_requested = 0;
                // notify tusb of completion
                dcd_event_xfer_complete(0, 0, bytes_received, XFER_RESULT_SUCCESS, true);
            } else {
                // store excess data
                while ((bytes_stored < 64) && (~(*CTL_CTRL) & (1 << 7))) {
                    rx_buffer[bytes_stored++] = *CTL_FIFO;
                }
            }
        } else {
            // empty, but we got a receive interrupt!
            // maybe this is a ZLP?
            dcd_event_xfer_complete(0, 0, 0, XFER_RESULT_SUCCESS, true);
        }
    }
    if (int_src & 0x80) {
        // Reset on control endpoint
        // happens on plugin after unplug
        // and also if you fire a "reset" event from the host (e.g. via libusb)

        // reset control endpoint as suggested by TinyUSB docs
        *CTL_CTRL &= 0xfd;   // put ctl interface in receive mode
        *CTL_CTRL = 0x10;
        *CTL_CTRL = 0;

        // now tell the stack what happened
        dcd_event_bus_signal(0, DCD_EVENT_BUS_RESET, true);
    }

    *IE |= 0x01;   // EX0 = 1

}

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
    if (ep_addr == 0x80) {

        // control endpoint, direction IN (toward host), i.e. our output
        *CTL_CTRL |= 0x02;   // put control endpoint in send mode

        // atomic section (w.r.t. transmission)
        // the Tx interrupt accesses these same values
        *INTENR0 &= ~((uint8_t)0x01);   // disable Tx complete interrupt
        remaining_data = buffer;
        bytes_remaining = total_bytes;

        bytes_sent = fill_xmit_fifo(remaining_data, bytes_remaining);
        // the HW may be much faster than our emulated software and can
        // potentially empty the FIFO and update bytes_remaining before we get here:
        bytes_remaining -= bytes_sent;
        remaining_data += bytes_sent;

        if ((total_bytes < 64) && (bytes_remaining == 0)) {
            // hit the send button
            *CTL_CTRL |= (uint8_t)(1 << 5);
        }
        *INTENR0 |= 0x01;   // enable Tx complete interrupt

        return true;   // means "no errors", not "complete"
    } else if (ep_addr == 0) {
        // direction OUT (receiving)

        *INTENR0 &= ~((uint8_t)0x02);   // disable Rx complete interrupt

        if ((bytes_stored > 0) && (total_bytes > 0)) {
            // serve this request immediately from a prior interrupt
            uint8_t xfer_size = (bytes_stored > total_bytes) ? total_bytes : bytes_stored;
            memcpy(buffer, rx_buffer, xfer_size);
            dcd_event_xfer_complete(0, 0, xfer_size, XFER_RESULT_SUCCESS, false);
            bytes_stored = 0;  // I guess we are discarding the remainder of the data here... TODO reconsider
        } else {
            *CTL_CTRL &= 0xfd;   // put control endpoint in receive mode

            // record request
            tusb_rcv_buffer = buffer;
            bytes_requested = total_bytes;
        }

        *INTENR0 |= 0x02;   // enable Rx complete interrupt

        return true;
    } else {
        // there will be other cases
        TU_BREAKPOINT();
    }
    return false;
}

void dcd_edpt_stall (uint8_t rhport, uint8_t ep_addr) {
    *CTL_CTRL |= 0x01;
}

void dcd_edpt_clear_stall     (uint8_t rhport, uint8_t ep_addr) {
    *CTL_CTRL &= 0xfe;
}

#endif  // M5623, the only one for now