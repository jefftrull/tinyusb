/* 
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bsp/board.h"
#include "tusb.h"

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF PROTYPES
//--------------------------------------------------------------------+

/* Blink pattern
 * - 250 ms  : device not mounted
 * - 1000 ms : device mounted
 * - 2500 ms : device is suspended
 */
enum  {
  BLINK_NOT_MOUNTED = 250,
  BLINK_MOUNTED = 1000,
  BLINK_SUSPENDED = 2500,
};

static uint32_t blink_interval_ms = BLINK_NOT_MOUNTED;

void led_blinking_task(void);

// tell SDCC this exists so it will create an interrupt table entry
void timer2_isr (void) __interrupt (5);

/*------------- MAIN -------------*/
int main(void)
{
  board_init();

  // init device stack on configured roothub port
  tud_init(BOARD_TUD_RHPORT);

  while (1)
  {
    tud_task(); // tinyusb device task
    led_blinking_task();

  }

  return 0;
}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void)
{
  blink_interval_ms = BLINK_MOUNTED;
}

// Invoked when device is unmounted
void tud_umount_cb(void)
{
  blink_interval_ms = BLINK_NOT_MOUNTED;
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en)
{
  (void) remote_wakeup_en;
  blink_interval_ms = BLINK_SUSPENDED;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void)
{
  blink_interval_ms = BLINK_MOUNTED;
}


//--------------------------------------------------------------------+
// BLINKING TASK
//--------------------------------------------------------------------+
void led_blinking_task(void)
{
  static uint32_t start_ms = 0;
  static bool led_state = false;

  // Blink every interval ms
  if ( board_millis() - start_ms < blink_interval_ms) return; // not enough time
  start_ms += blink_interval_ms;

  board_led_write(led_state);
  led_state = 1 - led_state; // toggle
}

// not listed as required in docs, but not WEAK either
// "close all non-control endpoints, cancel all pending transfers"
void dcd_edpt_close_all (uint8_t rhport) {}

// marked optional/WEAK but usbd.c does not check if it exists before calling
bool dcd_edpt_xfer_fifo       (uint8_t rhport, uint8_t ep_addr, tu_fifo_t * ff, uint16_t total_bytes) {
    return false;  // ?
}

// Not shown in docs, but not WEAK either
void dcd_sof_enable(uint8_t rhport, bool en) {}

//
// Optional functions
//

// sdcc does not support "weak" symbols you can test against to see if they are present
// so here I put in all the missing "optional" functions here as no-ops
#pragma disable_warning 85 // the parameters will be unused

// the one DCD function that is optional/WEAK and also checked accordingly
void dcd_edpt0_status_complete(uint8_t rhport, tusb_control_request_t const * request) {}

bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, CFG_TUSB_MEM_SECTION tusb_control_request_t const * request) {
    return false;   // stalls (reports error) if called, since we don't support
}

uint8_t tud_msc_get_maxlun_cb(void) { return 1; }   // 1 is default anyway so this is NOP

void tud_msc_read10_complete_cb(uint8_t lun) {}

uint8_t const * tud_descriptor_bos_cb(void) {
    // this is kind of bad, actually.
    // we don't have weak symbols but if this function exists usbd.c will use the
    // return value as a pointer into stuff
    TU_ASSERT(false);
    return NULL;
}

void tud_msc_write10_complete_cb(uint8_t lun) {}

int32_t tud_msc_request_sense_cb(uint8_t lun, void* buffer, uint16_t bufsize) {
    // another problem. we must supply this function, but then its result will be used
    TU_ASSERT(false);
    return sizeof(scsi_sense_fixed_resp_t);    // this seems to be the no-op result
}

#include "device/usbd_pvt.h"

usbd_class_driver_t const* usbd_app_driver_get_cb(uint8_t* driver_count) {
    // this is bad too. The mere existence of this function means the result
    // is dereferenced
    TU_ASSERT(false);
    return NULL;
}

void tud_msc_scsi_complete_cb(uint8_t lun, uint8_t const scsi_cmd[16]) {}
