/* GNU __extension__ keywords are accepted in any language mode; they
   borrow GCC internals (long long, statement expressions) that SDCC
   otherwise accepts directly, so the keyword is a no-op here. */

__extension__ typedef unsigned long extension_u32;
__extension__ typedef unsigned long long extension_u64;

__extension__ static unsigned char extension_glob = 3;

unsigned char
mcs251_extension_probe (unsigned char value)
{
  /* __extension__ before an expression is a silent no-op. */
  return __extension__ (value + 1);
}