/*-------------------------------------------------------------------------
   features.h - MCS-251 specific library features

   This library is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2 or (at your option) any later
   version.

   As a special exception, if you link this library with other files, some
   of which are compiled with SDCC, to produce an executable, this library
   does not by itself cause the resulting executable to be covered by the
   GNU General Public License.
-------------------------------------------------------------------------*/

#ifndef __SDCC_ASM_MCS251_FEATURES_H
#define __SDCC_ASM_MCS251_FEATURES_H 1

#define _REENTRANT __reentrant
#define _CODE __code

#if defined(__SDCC_STACK_AUTO)
  #if defined(__SDCC_USE_XSTACK)
    #define _AUTOMEM __pdata
  #else
    /* MCS251 stack objects use the complete 16-bit SPX and their address is a
       three-byte flat region-00 pointer, not an MCS-51 page-zero pointer. */
    #define _AUTOMEM __far
  #endif
#elif defined(__SDCC_MODEL_SMALL)
  #define _AUTOMEM __data
#elif defined(__SDCC_MODEL_MEDIUM)
  #define _AUTOMEM __pdata
#else
  #define _AUTOMEM __xdata
#endif

#if defined(__SDCC_MODEL_SMALL)
  #define _STATMEM __data
#elif defined(__SDCC_MODEL_MEDIUM)
  #define _STATMEM __pdata
#else
  #define _STATMEM __xdata
#endif

/* ECALL creates a 24-bit return frame, so inline assembly must use ERET. */
#define _RETURN eret

/* MCS251 uses a flat 24-bit code address space rather than banked calls. */
#define __SDCC_NONBANKED

#endif
