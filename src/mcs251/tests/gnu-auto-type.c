struct gnu_auto_record
{
  int value;
};

static __auto_type gnu_auto_long = 0x12345678L;
static int gnu_auto_counter;
static int gnu_auto_shadow = 23;

_Static_assert (
  __builtin_types_compatible_p (__typeof__ (gnu_auto_long), long),
  "__auto_type preserves a long initializer type");
int
gnu_auto_type_probe (void)
{
  __auto_type value = (unsigned long) gnu_auto_long;
  __auto_type pointer = &value;
  __auto_type record = (struct gnu_auto_record) { 17 };
  __auto_type evaluated_once = ++gnu_auto_counter;
  __auto_type gnu_auto_shadow = gnu_auto_shadow;
  register __auto_type register_auto = 5;
  const __auto_type qualified_auto = 7;
  auto int ordinary_auto = 9;

  _Static_assert (
    __builtin_types_compatible_p (__typeof__ (value), unsigned long),
    "local __auto_type preserves integer signedness");
  _Static_assert (
    __builtin_types_compatible_p (__typeof__ (pointer),
                                  __typeof__ (&value)),
    "local __auto_type preserves pointer types");
  _Static_assert (
    __builtin_types_compatible_p (__typeof__ (record),
                                  struct gnu_auto_record),
    "local __auto_type preserves structure types");

  return *pointer == 0x12345678UL && record.value == 17 &&
    evaluated_once == 1 && gnu_auto_shadow == 23 &&
    register_auto == 5 && qualified_auto == 7 && ordinary_auto == 9;
}
