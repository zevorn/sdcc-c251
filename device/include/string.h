/*-------------------------------------------------------------------------
   string.h - ISO header for string library functions

   Copyright (C) 1998, Sandeep Dutta
   Copyright (C) 2009-2025, Philipp Klaus Krause pkk@spth.de, philipp@colecovision.eu
   Copyright (C) 2025, Gabriele Gorla

   This library is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

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

#ifndef __SDCC_STRING_H
#define __SDCC_STRING_H 1

#ifndef NULL
# define NULL (void *)0
#endif

#ifndef __SIZE_T_DEFINED
# define __SIZE_T_DEFINED
  typedef unsigned int size_t;
#endif

/* Bounds-checking interfaces from annex K of the C11 standard. */
#if defined (__STDC_WANT_LIB_EXT1__) && __STDC_WANT_LIB_EXT1__

#ifndef __RSIZE_T_DEFINED
#define __RSIZE_T_DEFINED
typedef size_t rsize_t;
#endif

#ifndef __ERRNO_T_DEFINED
#define __ERRNO_T_DEFINED
typedef int errno_t;
#endif

#endif

#if defined(__SDCC_mcs51) || defined(__SDCC_ds390) || defined(__SDCC_mos6502) || defined(__SDCC_mos65c02)
#define __SDCC_BROKEN_STRING_FUNCTIONS
#endif

#if defined(__SDCC_mos6502) && !defined(__SDCC_STACK_AUTO)
#define _NEAR __zp
#else
#define _NEAR
#endif

// The function prototypes are ordered as in the ISO C11 standard.

// C90 Copying functions (ISO C23 7.28.2)

#if __STDC_VERSION__ >= 199901L
void *memccpy (void *restrict dst, const void *restrict src, int c, size_t n);
#endif

#if __STDC_VERSION__ >= 199901L
extern void *memcpy (void *restrict dest, const void *restrict src, size_t n);
#else
extern void *memcpy (void *dest, const void *src, size_t n);
#endif

#if defined(__SDCC_z80) || defined(__SDCC_z180) || defined(__SDCC_r2k) || defined(__SDCC_r2ka) || defined(__SDCC_r3ka) || defined(__SDCC_r4k) || defined(__SDCC_r5k) || defined(__SDCC_r6k)
extern void *memmove (void *dest, const void *src, size_t n) __preserves_regs(iyl, iyh);
#else
extern void *memmove (void *dest, const void *src, size_t n);
#endif

#if defined(__SDCC_z80) || defined (__SDCC_z80n) || defined(__SDCC_z180) || defined(__SDCC_r2k) || defined(__SDCC_r2ka) || defined(__SDCC_r3ka) || defined(__SDCC_r4k) || defined(__SDCC_r5k) || defined(__SDCC_r6k) || defined(__SDCC_tlcs90) || defined (__SDCC_ez80) || defined(__SDCC_r800)
#if __STDC_VERSION__ >= 199901L
extern char *strcpy (char dest[restrict static 1], const char src[restrict static 1]) __preserves_regs(iyl, iyh);
#else
extern char *strcpy (char *dest, const char *src) __preserves_regs(iyl, iyh);
#endif
#else
#if __STDC_VERSION__ >= 199901L
extern char *strcpy (char dest[restrict static 1], const char src[restrict static 1]);
#else
extern char *strcpy (char *dest, const char *_NEAR src);
#endif

#endif
#if __STDC_VERSION__ >= 199901L
extern char *strncpy (char *restrict dest, const char *restrict src, size_t n);
#else
extern char *strncpy (char *dest, const char *src, size_t n);
#endif

// C23 Duplication functions
#if __STDC_VERSION__ >= 199901L
extern char *strdup (const char s[static 1]);
extern char *strndup (const char *s, size_t n);
#endif

// C90 Concatenation functions (ISO C23 7.28.3)
#if __STDC_VERSION__ >= 199901L
extern char *strcat (char dest[restrict static 1], const char src[restrict static 1]);
extern char *strncat (char dest[restrict static 1], const char *restrict src, size_t n);
#else
extern char *strcat (char *dest, const char *src);
extern char *strncat (char * dest, const char *src, size_t n);
#endif

// Comparison functions (ISo C23 7.28.4)
extern int memcmp (const void *s1, const void *_NEAR s2, _NEAR size_t n);
#if __STDC_VERSION__ >= 199901L
extern int strcmp (const char s1[static 1], const char s2[static 1]);
#else
extern int strcmp (const char *s1, const char *_NEAR s2);
#endif
#define strcoll(s1, s2) strcmp(s1, s2)
/*int strcoll(const char *s1, const char *s2) {return strcmp(s1, s2);}*/
extern int strncmp (const char *s1, const char *s2, size_t n);
extern size_t strxfrm (char *dest, const char *src, size_t n);

// Search functions (ISO C23 7.28.5)

// C90 memchr (ISO C23 7.28.5.2)
extern void *memchr (const void *s, int c, size_t n);

// C90 memchr (ISO C23 7.28.5.3)
#ifdef __SDCC_BROKEN_STRING_FUNCTIONS
#if __STDC_VERSION__ >= 199901L
extern char *strchr (const char s[static 1], _NEAR char c); // c should be int according to standard.
#else
extern char *strchr (const char *s, _NEAR char c); // c should be int according to standard.
#endif
#else
#if __STDC_VERSION__ >= 199901L
extern char *strchr (const char s[static 1], int c);
#else
extern char *strchr (const char *s, int c);
#endif
#endif

// C90 memchr (ISO C23 7.28.5.4)
#if __STDC_VERSION__ >= 199901L
extern size_t strcspn(const char *s, const char *reject);
#else
extern size_t strcspn(const char s[static 1], const char reject[static 1]);
#endif

// C90 memchr (ISO C23 7.28.5.5)
#if __STDC_VERSION__ >= 199901L
extern char *strpbrk(const char s[static 1], const char accept[static 1]);
#else
extern char *strpbrk(const char *s, const char *accept);
#endif

// C90 strrchr (ISO C23 7.28.5.6)
#ifdef __SDCC_BROKEN_STRING_FUNCTIONS
#if __STDC_VERSION__ >= 199901L
extern char *strrchr(const char s[static 1], _NEAR char c); // c should be int according to standard.
#else
extern char *strrchr(const char *s, _NEAR char c); // c should be int according to standard.
#endif
#else
#if __STDC_VERSION__ >= 199901L
extern char *strrchr(const char s[static 1], int c);
#else
extern char *strrchr(const char *s, int c);
#endif
#endif

// C90 strspn (ISO C23 7.28.5.7)
#if __STDC_VERSION__ >= 199901L
extern size_t strspn (const char s[static 1], const char accept[static 1]);
#else
extern size_t strspn (const char *s, const char *accept);
#endif

// C90 strstr (ISO C23 7.28.5.8)
#if __STDC_VERSION__ >= 199901L
extern char *strstr (const char haystack[static 1], const char needle[static 1]);
#else
extern char *strstr (const char *haystack, const char *needle);
#endif

// C90 strtok (ISO C23 7.28.5.9)
#if __STDC_VERSION__ >= 199901L
extern char *strtok (char *restrict str, const char delim[restrict static 1]);
#else
extern char *strtok (char *str, const char *delim);
#endif

// Miscanelleous functions (ISo C23 7.28.6)
#ifdef __SDCC_BROKEN_STRING_FUNCTIONS
extern void *memset (void *s, _NEAR unsigned char c, _NEAR size_t n); // c should be int according to standard.
#else
extern void *memset (void *s, int c, size_t n);
#endif

extern void *memset_explicit (void *s, int c, size_t n);

/* extern char *strerror(int errnum); */
#if defined(__SDCC_z80) || defined (__SDCC_z80n) || defined(__SDCC_z180) || defined(__SDCC_tlcs90) || defined(__SDCC_ez80) || defined(__SDCC_r800)
#if __STDC_VERSION__ >= 199901L
extern size_t strlen (const char s[static 1]) __preserves_regs(iyl, iyh);
#else
extern size_t strlen (const char *s) __preserves_regs(iyl, iyh);
#endif
#else
#if __STDC_VERSION__ >= 199901L
extern size_t strlen (const char s[static 1]);
#else
extern size_t strlen (const char *s);
#endif
#endif

/* C2y length function: */
extern size_t strnlen (const char *s, size_t n);

#ifdef __SDCC_ds390
extern void __xdata *memcpyx(void __xdata *, void __xdata *, int) __naked;
#endif

#if defined(__SDCC_z80) || defined (__SDCC_z80n) || defined(__SDCC_z180) || defined(__SDCC_r2k) || defined(__SDCC_r2ka) || defined(__SDCC_r3ka) || defined(__SDCC_r4k) || defined(__SDCC_r5k) || defined(__SDCC_r6k) || defined (__SDCC_ez80) || defined(__SDCC_r800)
#define memcpy(dst, src, n) __builtin_memcpy(dst, src, n)
#define strcpy(dst, src) __builtin_strcpy(dst, src)
#define strncpy(dst, src, n) __builtin_strncpy(dst, src, n)
#define strchr(s, c) __builtin_strchr(s, c)
#define memset(dst, c, n) __builtin_memset(dst, c, n)
#else
extern void *__memcpy (void *dest, const void *_NEAR src, _NEAR size_t n);
#define memcpy(dst, src, n) __memcpy(dst, src, n)
#endif

#endif

