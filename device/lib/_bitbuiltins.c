/*-------------------------------------------------------------------------
   _bitbuiltins.c - GNU-compatible bit-count support functions

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

#define DEFINE_CLZ(name, type)                                              \
  int                                                                       \
  name (type value)                                                         \
  {                                                                         \
    type bit = (type) 1 << (sizeof (value) * 8 - 1);                        \
    int count = 0;                                                          \
                                                                            \
    while (bit && !(value & bit))                                           \
      {                                                                     \
        count++;                                                            \
        bit >>= 1;                                                          \
      }                                                                     \
    return count;                                                           \
  }

#define DEFINE_CTZ(name, type)                                              \
  int                                                                       \
  name (type value)                                                         \
  {                                                                         \
    type bit = 1;                                                           \
    int count = 0;                                                          \
                                                                            \
    while (bit && !(value & bit))                                           \
      {                                                                     \
        count++;                                                            \
        bit <<= 1;                                                          \
      }                                                                     \
    return count;                                                           \
  }

#define DEFINE_POPCOUNT(name, type)                                         \
  int                                                                       \
  name (type value)                                                         \
  {                                                                         \
    int count = 0;                                                          \
                                                                            \
    while (value)                                                           \
      {                                                                     \
        count += value & 1;                                                 \
        value >>= 1;                                                        \
      }                                                                     \
    return count;                                                           \
  }

#define DEFINE_FFS(name, signed_type, unsigned_type)                         \
  int                                                                       \
  name (signed_type value)                                                  \
  {                                                                         \
    unsigned_type bits = (unsigned_type) value;                             \
    int position = 1;                                                       \
                                                                            \
    if (!bits)                                                              \
      return 0;                                                             \
    while (!(bits & 1))                                                     \
      {                                                                     \
        position++;                                                         \
        bits >>= 1;                                                         \
      }                                                                     \
    return position;                                                        \
  }

DEFINE_CLZ (__builtin_clz, unsigned int)
DEFINE_CLZ (__builtin_clzl, unsigned long)
DEFINE_CLZ (__builtin_clzll, unsigned long long)

DEFINE_CTZ (__builtin_ctz, unsigned int)
DEFINE_CTZ (__builtin_ctzl, unsigned long)
DEFINE_CTZ (__builtin_ctzll, unsigned long long)

DEFINE_POPCOUNT (__builtin_popcount, unsigned int)
DEFINE_POPCOUNT (__builtin_popcountl, unsigned long)
DEFINE_POPCOUNT (__builtin_popcountll, unsigned long long)

DEFINE_FFS (__builtin_ffs, int, unsigned int)
DEFINE_FFS (__builtin_ffsl, long, unsigned long)
DEFINE_FFS (__builtin_ffsll, long long, unsigned long long)

unsigned short
__builtin_bswap16 (unsigned short value)
{
  return (value << 8) | (value >> 8);
}

unsigned long
__builtin_bswap32 (unsigned long value)
{
  value = ((value & 0x00ff00fful) << 8) |
          ((value >> 8) & 0x00ff00fful);
  return (value << 16) | (value >> 16);
}

unsigned long long
__builtin_bswap64 (unsigned long long value)
{
  value = ((value & 0x00ff00ff00ff00ffull) << 8) |
          ((value >> 8) & 0x00ff00ff00ff00ffull);
  value = ((value & 0x0000ffff0000ffffull) << 16) |
          ((value >> 16) & 0x0000ffff0000ffffull);
  return (value << 32) | (value >> 32);
}
