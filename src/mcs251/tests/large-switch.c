unsigned long
mcs251_large_switch (unsigned long live, unsigned char selector)
{
  volatile unsigned char local_selector = selector;

  switch (local_selector)
    {
    case 0: return live + 17;
    case 1: return live + 18;
    case 2: return live + 19;
    case 3: return live + 20;
    case 4: return live + 21;
    case 5: return live + 22;
    case 6: return live + 23;
    case 7: return live + 24;
    case 8: return live + 25;
    case 9: return live + 26;
    case 10: return live + 27;
    case 11: return live + 28;
    case 12: return live + 29;
    case 13: return live + 30;
    case 14: return live + 31;
    case 15: return live + 32;
    case 16: return live + 33;
    case 17: return live + 34;
    case 18: return live + 35;
    case 19: return live + 36;
    default: return 0;
    }
}

unsigned long long
mcs251_shift_then_switch (unsigned long long value,
                          unsigned char selector)
{
  volatile unsigned long long shifted = value;

  shifted <<= 11;
  switch (selector)
    {
    case 0: return shifted + 1;
    case 1: return shifted + 2;
    case 2: return shifted + 3;
    case 3: return shifted + 4;
    case 4: return shifted + 5;
    case 5: return shifted + 6;
    case 6: return shifted + 7;
    case 7: return shifted + 8;
    default: return 0;
    }
}

unsigned long long
mcs251_right_shift_then_switch (unsigned long long value,
                                unsigned char selector)
{
  volatile unsigned long long shifted = value;

  shifted >>= 11;
  switch (selector)
    {
    case 0: return shifted + 9;
    case 1: return shifted + 10;
    case 2: return shifted + 11;
    case 3: return shifted + 12;
    case 4: return shifted + 13;
    case 5: return shifted + 14;
    case 6: return shifted + 15;
    case 7: return shifted + 16;
    default: return 0;
    }
}

#ifdef MCS251_WIDE_SHIFT_RUNTIME
__sfr __at (0x99) SBUF;

void
main (void)
{
  unsigned char passed = 1;
  const unsigned long long value = 0x0001020304050607ULL;

  if (mcs251_large_switch (0x12345678UL, 19) != 0x1234569cUL ||
      mcs251_large_switch (0x12345678UL, 20) != 0 ||
      mcs251_shift_then_switch (value, 5) !=
        0x0810182028303806ULL ||
      mcs251_right_shift_then_switch (value, 6) !=
        0x00000020406080afULL)
    passed = 0;

  if (passed)
    {
      SBUF = 'P';
      SBUF = 'A';
      SBUF = 'S';
      SBUF = 'S';
    }
  else
    {
      SBUF = 'F';
      SBUF = 'A';
      SBUF = 'I';
      SBUF = 'L';
    }
  SBUF = '\n';

  for (;;)
    {
    }
}
#endif
