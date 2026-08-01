/* Semantic source:
   https://www.keil.com/download/docs/47.asp

   This is an original bounded-vector test of the documented ROM-checksum
   behavior.  It does not copy the Keil example implementation or data. */

#include "case-support.h"

static const unsigned char __code checksum_bytes[] = {
    0x03, 0x11, 0x25, 0x42, 0x7e, 0x80, 0xa5, 0xff,
    0x5a, 0xc3, 0x0d, 0x10, 0x20, 0x40, 0x81, 0x02,
};

void
main (void)
{
    const unsigned char __code *cursor = checksum_bytes;
    unsigned int checksum = 0;
    unsigned char remaining = sizeof (checksum_bytes);

    while (remaining--)
        checksum += *cursor++;

    case_report ("official-rom-checksum", checksum == 0x053a);
    for (;;)
        {
        }
}
