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

volatile unsigned long mcs251_dword_values[3] =
  { 0x10203040ul, 0x50607080ul, 0x90a0b0c0ul };
volatile unsigned int mcs251_word_values[6] =
  { 0x1020u, 0x3040u, 0x5060u, 0x7080u, 0x90a0u, 0xb0c0u };

unsigned long
mcs251_dword_register_pressure (void)
{
  unsigned long value0 =
    mcs251_dword_values[0] + mcs251_dword_values[1];
  unsigned long value1 =
    mcs251_dword_values[2] ^ 0x01020304ul;

  return value0 + value1;
}

unsigned int
mcs251_word_register_pressure (unsigned char input)
{
  unsigned int value0 = mcs251_word_values[0] + mcs251_word_values[1];
  unsigned int value1 = mcs251_word_values[2] ^ mcs251_word_values[3];
  unsigned int value2 = mcs251_word_values[4] + mcs251_word_values[5];
  unsigned int widened = input;

  return value0 + value1 + value2 + widened;
}

unsigned long
mcs251_unsigned_word_multiply (unsigned int left, unsigned int right)
{
  return (unsigned long)left * right;
}

#if defined (__SDCC_mcs251)
unsigned int
mcs251_high_register_shift (unsigned int year, unsigned int day)
{
  unsigned int saved0 = mcs251_word_values[0];
  unsigned int saved1 = mcs251_word_values[1];
  unsigned int saved2 = mcs251_word_values[2];
  unsigned int result;

  result = year * 365u + year / 4u - year / 100u + day;

  return result + saved0 + saved1 + saved2;
}

unsigned char
mcs251_high_register_loop (void)
{
  unsigned char value0 = mcs251_pressure_values[0];
  unsigned char value1 = mcs251_pressure_values[1];
  unsigned char value2 = mcs251_pressure_values[2];
  unsigned char value3 = mcs251_pressure_values[3];
  unsigned char value4 = mcs251_pressure_values[4];
  unsigned char value5 = mcs251_pressure_values[5];
  unsigned char value6 = mcs251_pressure_values[6];
  unsigned char value7 = mcs251_pressure_values[7];

  value1 += value7;
  value7 ^= value0;
  value7 = value7 - 1;
  if (value7)
    value0 += value4;

  return value0 ^ value1 ^ value2 ^ value3 ^
         value4 ^ value5 ^ value6 ^ value7;
}
#endif

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
  if (mcs251_dword_register_pressure () != 0xf2235484ul)
    passed = 0;
  if (mcs251_word_register_pressure (0x5a) != 0xa2fau)
    passed = 0;
  if (mcs251_unsigned_word_multiply (0x1234u, 0xabcdu) !=
      0x0c374fa4ul)
    passed = 0;
#if defined (__SDCC_mcs251)
  if (mcs251_high_register_shift (369, 307) != 0xa069u)
    passed = 0;
  if (mcs251_high_register_loop () != 0x07)
    passed = 0;
#endif

  print_result (passed);
  for (;;)
    {
    }
}
#endif
