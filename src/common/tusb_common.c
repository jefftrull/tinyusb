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
 * This file is part of the TinyUSB stack.
 */

// put some functions here that will otherwise be in EVERY SINGLE MODULE
// thanks sdcc
#include "tusb_common.h"

 uint32_t tu_u32(uint8_t b3, uint8_t b2, uint8_t b1, uint8_t b0)
{
  return ( ((uint32_t) b3) << 24) | ( ((uint32_t) b2) << 16) | ( ((uint32_t) b1) << 8) | b0;
}

 uint16_t tu_u16(uint8_t high, uint8_t low)
{
  return (uint16_t) ((((uint16_t) high) << 8) | low);
}

 uint8_t tu_u32_byte3(uint32_t ui32) { return TU_U32_BYTE3(ui32); }
 uint8_t tu_u32_byte2(uint32_t ui32) { return TU_U32_BYTE2(ui32); }
 uint8_t tu_u32_byte1(uint32_t ui32) { return TU_U32_BYTE1(ui32); }
 uint8_t tu_u32_byte0(uint32_t ui32) { return TU_U32_BYTE0(ui32); }

 uint16_t tu_u32_high16(uint32_t ui32) { return (uint16_t) (ui32 >> 16); }
 uint16_t tu_u32_low16 (uint32_t ui32) { return (uint16_t) (ui32 & 0x0000ffffu); }

 uint8_t tu_u16_high(uint16_t ui16) { return TU_U16_HIGH(ui16); }
 uint8_t tu_u16_low (uint16_t ui16) { return TU_U16_LOW(ui16); }

//------------- Bits -------------//
 uint32_t tu_bit_set  (uint32_t value, uint8_t pos) { return value | TU_BIT(pos);                  }
 uint32_t tu_bit_clear(uint32_t value, uint8_t pos) { return value & (~TU_BIT(pos));               }
 bool     tu_bit_test (uint32_t value, uint8_t pos) { return (value & TU_BIT(pos)) ? true : false; }

//------------- Min -------------//
 uint8_t  tu_min8  (uint8_t  x, uint8_t y ) { return (x < y) ? x : y; }
 uint16_t tu_min16 (uint16_t x, uint16_t y) { return (x < y) ? x : y; }
 uint32_t tu_min32 (uint32_t x, uint32_t y) { return (x < y) ? x : y; }

//------------- Max -------------//
 uint8_t  tu_max8  (uint8_t  x, uint8_t y ) { return (x > y) ? x : y; }
 uint16_t tu_max16 (uint16_t x, uint16_t y) { return (x > y) ? x : y; }

//------------- Align -------------//
 uint32_t tu_align(uint32_t value, uint32_t alignment)
{
  return value & ((uint32_t) ~(alignment-1));
}

 uint32_t tu_align16 (uint32_t value) { return (value & 0xFFFFFFF0UL); }
 uint32_t tu_align32 (uint32_t value) { return (value & 0xFFFFFFE0UL); }
 uint32_t tu_align4k (uint32_t value) { return (value & 0xFFFFF000UL); }
 uint32_t tu_offset4k(uint32_t value) { return (value & 0xFFFUL); }

//------------- Mathematics -------------//
uint32_t tu_div_ceil(uint32_t v, uint32_t d) { return (v + d -1)/d; }

// log2 of a value is its MSB's position
// TODO use clz TODO remove
uint8_t tu_log2(uint32_t value)
{
  uint8_t result = 0;
  while (value >>= 1) { result++; }
  return result;
}

bool tu_is_power_of_two(uint32_t value)
{
   return (value != 0) && ((value & (value - 1)) == 0);
}
