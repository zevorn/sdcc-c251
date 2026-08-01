__sfr __at (0x98) SCON;
__sfr __at (0x99) SBUF;

#define SCON_TI 0x02

unsigned char
__sdcc_external_startup (void)
{
  SCON = 0;
  return 0;
}

void
_putchar (char value)
{
  SCON &= (unsigned char)~SCON_TI;
  SBUF = value;
  while (!(SCON & SCON_TI))
    {
    }
  SCON &= (unsigned char)~SCON_TI;
}

void
_initEmu (void)
{
}

void
_exitEmu (void)
{
  for (;;)
    {
    }
}
