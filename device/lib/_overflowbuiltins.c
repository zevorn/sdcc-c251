/*-------------------------------------------------------------------------
   _overflowbuiltins.c - GNU-compatible generic overflow helpers

   Copyright (C) 2026 Chao Liu <chao.liu.zevorn@gmail.com>

   This library is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this library; see the file COPYING. If not, write to the
   Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
   MA 02110-1301, USA.

   As a special exception, if you link this library with other files,
   some of which are compiled with SDCC, to produce an executable,
   this library does not by itself cause the resulting executable to
   be covered by the GNU General Public License. This exception does
   not however invalidate any other reasons why the executable file
   might be covered by the GNU General Public License.
-------------------------------------------------------------------------*/

#include <stdbit.h>

#define OVERFLOW_ADD 1
#define OVERFLOW_SUB 2
#define OVERFLOW_MUL 3

static unsigned long long
magnitude_from_bits (unsigned long long bits, unsigned char width,
                     unsigned char is_signed,
                     unsigned char *negative)
{
  unsigned long long mask = ~0ULL;

  if (width < 64)
    {
      mask = (1ULL << width) - 1;
      bits &= mask;
    }
  *negative = is_signed && !!(bits & (1ULL << (width - 1)));
  if (*negative)
    bits = (0ULL - bits) & mask;
  return bits;
}

static void
store_result (unsigned long long magnitude, unsigned char negative,
              volatile void *result, unsigned char width)
{
  volatile unsigned char *bytes = result;
  unsigned char count = width / 8;

  if (negative)
    magnitude = 0ULL - magnitude;
  for (unsigned char i = 0; i < count; i++)
    {
#if __STDC_ENDIAN_NATIVE__ == __STDC_ENDIAN_BIG__
      bytes[count - i - 1] = magnitude;
#else
      bytes[i] = magnitude;
#endif
      magnitude >>= 8;
    }
}

_Bool
__sdcc_overflow (unsigned char operation,
                 unsigned long long left_bits,
                 unsigned char left_width,
                 unsigned char left_signed,
                 unsigned long long right_bits,
                 unsigned char right_width,
                 unsigned char right_signed,
                 volatile void *result_pointer,
                 unsigned char result_width,
                 unsigned char result_signed) __reentrant
{
  unsigned long long left;
  unsigned long long right;
  unsigned char left_negative;
  unsigned char right_negative;
  unsigned char result_negative;
  _Bool overflow = 0;

  left = magnitude_from_bits (left_bits, left_width, left_signed,
                              &left_negative);
  right = magnitude_from_bits (right_bits, right_width, right_signed,
                               &right_negative);
  if (operation == OVERFLOW_SUB && right)
    right_negative = !right_negative;

  if (operation == OVERFLOW_ADD || operation == OVERFLOW_SUB)
    {
      if (left_negative == right_negative)
        {
          overflow = right > ~0ULL - left;
          left += right;
          result_negative = left_negative;
        }
      else if (left >= right)
        {
          left -= right;
          result_negative = left_negative;
        }
      else
        {
          left = right - left;
          result_negative = right_negative;
        }
    }
  else
    {
      overflow = left && right > ~0ULL / left;
      left *= right;
      result_negative = left_negative != right_negative;
    }

  if (!left)
    result_negative = 0;
  store_result (left, result_negative, result_pointer, result_width);
  if (!result_signed)
    {
      if (result_negative && left)
        overflow = 1;
      else if (result_width < 64 &&
               left >= (1ULL << result_width))
        overflow = 1;
    }
  else
    {
      right = 1ULL << (result_width - 1);
      if (result_negative ? left > right : left >= right)
        overflow = 1;
    }
  return overflow;
}
