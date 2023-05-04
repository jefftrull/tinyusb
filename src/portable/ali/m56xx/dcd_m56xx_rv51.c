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
// Control endpoint
static uint16_t ctl_bytes_sent = 0;       // for remembering how many bytes we put in the fifo
static uint8_t const * ctl_remaining_data = NULL;
static uint16_t ctl_bytes_remaining = 0;  // data count not yet placed in Tx fifo

// Bulk IN
static uint16_t bulk_bytes_sent = 0;
static uint8_t const * bulk_remaining_data = NULL;
static uint16_t bulk_bytes_remaining = 0;

// Rx bookkeeping
// Control endpoint
static uint8_t ctl_bytes_requested = 0;
static uint8_t * tusb_ctl_rcv_buffer = NULL;

// Bulk OUT
static uint8_t bulk_bytes_requested = 0;
static uint8_t * tusb_bulk_rcv_buffer = NULL;

// Temporary storage for data received prior to a request from TUSB
static uint16_t ctl_bytes_stored = 0;
static uint8_t ctl_rx_buffer[64];

static uint16_t bulk_bytes_stored = 0;
static uint8_t bulk_rx_buffer[512];

// for debugging
void usb_intr_isr(void);  // in board code cs4200f.c
bool usb_intr_putc(char c);
bool usb_intr_putc_hex(char c);
bool usb_intr_puts(char const * s);
bool usb_intr_put_req(tusb_control_request_t const * req);

//
// Transmit FIFO handling
//

uint8_t ctl_fill_xmit_fifo(uint8_t const * buffer, uint16_t buffer_size) {
    uint8_t send_count = 0;

    while ((send_count < buffer_size) && (~(*CTL_CTRL) & (1 << 6))) {
        // FIFO not full and we still have data
        *CTL_FIFO = *buffer++;
        ++send_count;
    }

    return send_count;
}

uint16_t bulk_fill_xmit_fifo(uint8_t const * buffer, uint16_t buffer_size) {
    uint16_t send_count = 0;

    while ((send_count < buffer_size) && (~(*BLKI_CTRL) & (1 << 6))) {
        // FIFO not full and we still have data
        *BLKI_FIFO = *buffer++;
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

        if (!first_tx && (ctl_bytes_remaining != 0)) {
            // send another chunk

            bool short_transmission = (ctl_bytes_remaining < 64);

            uint8_t packet_size = ctl_fill_xmit_fifo(ctl_remaining_data, ctl_bytes_remaining);
            ctl_bytes_remaining -= packet_size;
            ctl_remaining_data += packet_size;
            ctl_bytes_sent += packet_size;
            if (short_transmission) {
                *CTL_CTRL |= (uint8_t)(1 << 5);  // force transmit
            }

        } else if (!first_tx && (ctl_bytes_remaining == 0)) {
            // all data en route to host; inform TUSB (which may give us more)
            dcd_event_xfer_complete(0, 0x80, ctl_bytes_sent, XFER_RESULT_SUCCESS, true);
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
            if (ctl_bytes_requested != 0) {
                uint16_t bytes_received = 0;   // for remembering how many bytes we took from the fifo
                while ((bytes_received < ctl_bytes_requested) && (~(*CTL_CTRL) & (1 << 7))) {
                    // FIFO not empty and there is space remaining
                    *tusb_ctl_rcv_buffer++ = *CTL_FIFO;
                    ++bytes_received;
                }
                // clear pending request
                ctl_bytes_requested = 0;
                // notify tusb of completion
                dcd_event_xfer_complete(0, 0, bytes_received, XFER_RESULT_SUCCESS, true);
            } else {
                // store excess data
                while ((ctl_bytes_stored < 64) && (~(*CTL_CTRL) & (1 << 7))) {
                    ctl_rx_buffer[ctl_bytes_stored++] = *CTL_FIFO;
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

    if (int_src & 0x08) {
        // rcv complete BLKO

        if ((*BLKO_CTRL) & 0x80) {
            // empty FIFO but we got a data receive interrupt
            dcd_event_xfer_complete(0, 0x02, 0, XFER_RESULT_SUCCESS, true); // ZLP?
        } else {
            if (bulk_bytes_requested != 0) {
                // tusb previously asked us to do a read
                uint16_t bytes_received = 0;
                while ((bytes_received < bulk_bytes_requested) && ((~(*BLKO_CTRL) & (1 << 7)))) {
                    *tusb_bulk_rcv_buffer++ = *BLKO_FIFO;
                    ++bytes_received;
                }
                bulk_bytes_requested = 0;  // clear pending request
                dcd_event_xfer_complete(0, 0x02, bytes_received, XFER_RESULT_SUCCESS, true);
            } else {
                // store this excess data in the hope tusb will ask for it
                uint16_t bytes_received = 0;
                while ((bulk_bytes_stored < 512) && (~(*BLKO_CTRL) & (1 << 7))) {
                    bulk_rx_buffer[bulk_bytes_stored++] = *BLKO_FIFO;
                    ++bytes_received;
                }
            }
        }
    }

    if (int_src & 0x04) {
        // transmit complete on BLKI
        if (bulk_bytes_remaining != 0) {
            bool short_transmission = (bulk_bytes_remaining < 512);

            uint8_t packet_size = bulk_fill_xmit_fifo(bulk_remaining_data, bulk_bytes_remaining);
            bulk_bytes_remaining -= packet_size;
            bulk_remaining_data += packet_size;
            bulk_bytes_sent += packet_size;
            if (short_transmission) {
                *BLKI_CTRL |= (uint8_t)(1 << 5);  // force transmit
            }
        } else {
            dcd_event_xfer_complete(0, 0x81, bulk_bytes_sent, XFER_RESULT_SUCCESS, true);
        }
    }

    *IE |= 0x01;   // EX0 = 1

}

// These stubs did not require any implementation for MSC to work
void dcd_set_address(uint8_t rhport, uint8_t dev_addr) {}
void dcd_remote_wakeup(uint8_t rhport) {}
void dcd_connect(uint8_t rhport) {}
void dcd_disconnect(uint8_t rhport) {}
bool dcd_edpt_open(uint8_t rhport, tusb_desc_endpoint_t const * desc_ep) {
    return true;
}
void dcd_edpt_close (uint8_t rhport, uint8_t ep_addr) {}

bool dcd_edpt_xfer (uint8_t rhport, uint8_t ep_addr, uint8_t * buffer, uint16_t total_bytes) {
    if (ep_addr == 0x80) {

        // control endpoint, direction IN (toward host), i.e. our output
        *CTL_CTRL |= 0x02;   // put control endpoint in send mode

        // atomic section (w.r.t. transmission)
        // the Tx interrupt accesses these same values
        *INTENR0 &= ~((uint8_t)0x01);   // disable Tx complete interrupt
        ctl_remaining_data = buffer;
        ctl_bytes_remaining = total_bytes;

        ctl_bytes_sent = ctl_fill_xmit_fifo(ctl_remaining_data, ctl_bytes_remaining);
        // the HW may be much faster than our emulated software and can
        // potentially empty the FIFO and update ctl_bytes_remaining before we get here:
        ctl_bytes_remaining -= ctl_bytes_sent;
        ctl_remaining_data += ctl_bytes_sent;

        if ((total_bytes < 64) && (ctl_bytes_remaining == 0)) {
            // hit the send button
            *CTL_CTRL |= (uint8_t)(1 << 5);
        }
        *INTENR0 |= 0x01;   // enable Tx complete interrupt

        return true;   // means "no errors", not "complete"
    } else if (ep_addr == 0) {
        // direction OUT (receiving)

        *INTENR0 &= ~((uint8_t)0x02);   // disable Rx complete interrupt

        if ((ctl_bytes_stored > 0) && (total_bytes > 0)) {
            // serve this request immediately from a prior interrupt
            uint8_t xfer_size = (ctl_bytes_stored > total_bytes) ? total_bytes : ctl_bytes_stored;
            memcpy(buffer, ctl_rx_buffer, xfer_size);
            dcd_event_xfer_complete(0, 0, xfer_size, XFER_RESULT_SUCCESS, false);
            ctl_bytes_stored = 0;  // I guess we are discarding the remainder of the data here... TODO reconsider
        } else {
            *CTL_CTRL &= 0xfd;   // put control endpoint in receive mode

            // record request
            tusb_ctl_rcv_buffer = buffer;
            ctl_bytes_requested = total_bytes;
        }

        *INTENR0 |= 0x02;   // enable Rx complete interrupt

        return true;
    } else if (ep_addr == 0x02) {
        // bulk OUT (receiving)

        // This bit of DMACTR is documented for the 5621 as "DMA operation direction"
        // but it needs to be set appropriately regardless of whether we use DMA
        *DMACTR &= ~((uint8_t)0x04);    // put bulk endpoints into receive mode (I think)

        *INTENR0 &= ~((uint8_t)0x08);   // disable bulk receive interrupts

        if ((bulk_bytes_stored > 0) && (total_bytes > 0)) {
            // serve this request immediately from a prior interrupt
            uint8_t xfer_size = (bulk_bytes_stored > total_bytes) ? total_bytes : bulk_bytes_stored;
            memcpy(buffer, bulk_rx_buffer, xfer_size);
            dcd_event_xfer_complete(0, 0x02, xfer_size, XFER_RESULT_SUCCESS, false);
            bulk_bytes_stored = 0;
        } else {
            // record request
            tusb_bulk_rcv_buffer = buffer;
            bulk_bytes_requested = total_bytes;
        }

        *INTENR0 |= 0x08;               // enable bulk receive interrupts

        return true;
    } else if (ep_addr == 0x81) {
        // we do sometimes see exactly 512B here
        // we don't seem to ever send ZLP though

        *DMACTR |= 0x04;               // put bulk endpoints into transmit mode

        *INTENR0 &= ~((uint8_t)0x04);
        bulk_remaining_data = buffer;
        bulk_bytes_remaining = total_bytes;

        bulk_bytes_sent = bulk_fill_xmit_fifo(bulk_remaining_data, bulk_bytes_remaining);
        bulk_bytes_remaining -= bulk_bytes_sent;
        bulk_remaining_data += bulk_bytes_sent;

        if ((total_bytes < 512) && (bulk_bytes_remaining == 0)) {
            *BLKI_CTRL |= (uint8_t)(1 << 5);
        }

        *INTENR0 |= 0x04;

        return true;
    } else {
        // This shouldn't happen
        TU_BREAKPOINT();
    }
    return false;
}

void dcd_edpt_stall (uint8_t rhport, uint8_t ep_addr) {
    if ((ep_addr & 0x7f) == 0) {
        *CTL_CTRL |= 0x01;
    } else if ((ep_addr & 0x7f) == 1) {
        *BLKI_CTRL |= 0x01;
    } else if ((ep_addr & 0x7f) == 2) {
        *BLKO_CTRL |= 0x01;
    } else {
        // should not happen
        TU_BREAKPOINT();
    }
}

void dcd_edpt_clear_stall (uint8_t rhport, uint8_t ep_addr) {
    if ((ep_addr & 0x7f) == 0) {
        *CTL_CTRL &= 0xfe;
    } else if ((ep_addr & 0x7f) == 1) {
        *BLKI_CTRL &= 0xfe;
    } else if ((ep_addr & 0x7f) == 2) {
        *BLKO_CTRL &= 0xfe;
    } else {
        // should not happen
        TU_BREAKPOINT();
    }
}

#endif  // M5623, the only one for now
