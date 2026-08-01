#ifndef SDCC_MCS251_TEST_CASE_SUPPORT_H
#define SDCC_MCS251_TEST_CASE_SUPPORT_H

#include "stc32g-qemu.h"

static void
case_serial_putc (unsigned char value)
{
    SCON &= (unsigned char)~SCON_TI;
    SBUF = value;
    while (!(SCON & SCON_TI))
        {
        }
    SCON &= (unsigned char)~SCON_TI;
}

static void
case_serial_puts (const char __code *text)
{
    while (*text)
        case_serial_putc (*text++);
}

static void
case_serial_puthex (unsigned char value)
{
    unsigned char nibble = value >> 4;

    case_serial_putc (nibble < 10 ? nibble + '0' : nibble - 10 + 'a');
    nibble = value & 0x0f;
    case_serial_putc (nibble < 10 ? nibble + '0' : nibble - 10 + 'a');
}

static void
case_report (const char __code *name, unsigned char passed)
{
    case_serial_puts ("CASE:");
    case_serial_puts (name);
    case_serial_putc (':');
    case_serial_puts (passed ? "PASS\n" : "FAIL\n");
}

#endif
