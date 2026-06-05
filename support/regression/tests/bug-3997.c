/*
   bug-3997.c
   An issue in z80 code generation resulting in compilation failing
 */

#include <testfwk.h>

#if !defined(__SDCC_z80) && !defined(__SDCC_sm83)
#define __sfr char
#endif

#define BANK(VARNAME) ( (unsigned char) ((unsigned int)(& __bank_ ## VARNAME)) )

#define BANKREF_EXTERN(VARNAME) const char __bank_ ## VARNAME;
#define __REG volatile __sfr
#define __BYTE_REG volatile unsigned char
#define SWITCH_ROM(b) (_current_bank = (b), rROMB0 = (b))

__BYTE_REG rROMB0;
__REG _current_bank;

extern const unsigned char some_const_var_4;
BANKREF_EXTERN(some_const_var_4)

void f(void)
{
  unsigned char _saved_bank;
  _saved_bank = _current_bank;
  SWITCH_ROM(BANK(some_const_var_4));
  // ...
  SWITCH_ROM(_saved_bank);
}

void
testBug(void)
{
}

