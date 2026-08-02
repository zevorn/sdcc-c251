volatile unsigned char mcs251_pressure_values[12] =
  { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };

unsigned char
mcs251_byte_register_pressure (unsigned char iterations)
{
  unsigned char value0 = 0;
  unsigned char value1 = 1;
  unsigned char value2 = 2;
  unsigned char value3 = 3;
  unsigned char value4 = 4;
  unsigned char value5 = 5;
  unsigned char value6 = 6;
  unsigned char value7 = 7;
  unsigned char value8 = 8;
  unsigned char value9 = 9;
  unsigned char value10 = 10;
  unsigned char value11 = 11;

  while (iterations--)
    {
      value0 += mcs251_pressure_values[0];
      value1 += mcs251_pressure_values[1];
      value2 += mcs251_pressure_values[2];
      value3 += mcs251_pressure_values[3];
      value4 += mcs251_pressure_values[4];
      value5 += mcs251_pressure_values[5];
      value6 += mcs251_pressure_values[6];
      value7 += mcs251_pressure_values[7];
      value8 += mcs251_pressure_values[8];
      value9 += mcs251_pressure_values[9];
      value10 += mcs251_pressure_values[10];
      value11 += mcs251_pressure_values[11];
    }

  if (value11 == 0xff)
    return 0;

  return value0 ^ value1 ^ value2 ^ value3 ^ value4 ^ value5 ^
         value6 ^ value7 ^ value8 ^ value9 ^ value10 ^ value11;
}

static void
mcs251_pressure_clobber (void)
{
  ++mcs251_pressure_values[0];
}

unsigned char
mcs251_byte_register_call_pressure (void)
{
  unsigned char value0 = mcs251_pressure_values[0];
  unsigned char value1 = mcs251_pressure_values[1];
  unsigned char value2 = mcs251_pressure_values[2];
  unsigned char value3 = mcs251_pressure_values[3];
  unsigned char value4 = mcs251_pressure_values[4];
  unsigned char value5 = mcs251_pressure_values[5];
  unsigned char value6 = mcs251_pressure_values[6];
  unsigned char value7 = mcs251_pressure_values[7];
  unsigned char value8 = mcs251_pressure_values[8];
  unsigned char value9 = mcs251_pressure_values[9];
  unsigned char value10 = mcs251_pressure_values[10];
  unsigned char value11 = mcs251_pressure_values[11];

  mcs251_pressure_clobber ();
  return value0 ^ value1 ^ value2 ^ value3 ^ value4 ^ value5 ^
         value6 ^ value7 ^ value8 ^ value9 ^ value10 ^ value11;
}

void
mcs251_register_pressure_isr (void) __interrupt (1)
{
  mcs251_pressure_values[0] = mcs251_byte_register_pressure (2);
}

#if defined (MCS_REGISTER_PRESSURE_RUNTIME)
__sfr __at (0x99) SBUF;

static void
print_result (unsigned char passed)
{
  const char *text = passed ? "PASS\n" : "FAIL\n";

  while (*text)
    SBUF = *text++;
}

void
main (void)
{
  unsigned char passed = mcs251_byte_register_call_pressure () == 0x0c;

  mcs251_pressure_values[0] = 1;
  if (mcs251_byte_register_pressure (2) != 0x1c)
    passed = 0;

  print_result (passed);
  for (;;)
    {
    }
}
#endif
