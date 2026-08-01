int gnu_constant_p_counter;
volatile int gnu_constant_p_volatile;

int gnu_constant_p_runtime_value (void);

_Static_assert (__builtin_constant_p (17),
                "integer literals are constant");
_Static_assert (__builtin_constant_p (3 * 7 + 1),
                "folded arithmetic is constant");
_Static_assert (__builtin_constant_p (sizeof (gnu_constant_p_counter)),
                "sizeof results are constant");
_Static_assert (!__builtin_constant_p (gnu_constant_p_volatile),
                "volatile object values are not constant");
_Static_assert (!__builtin_constant_p (++gnu_constant_p_counter),
                "side-effecting expressions are not constant");
_Static_assert (!__builtin_constant_p (gnu_constant_p_counter = 1),
                "assignments are not constant");
_Static_assert (!__builtin_constant_p (gnu_constant_p_runtime_value ()),
                "function calls are not constant");

int
gnu_constant_p_side_effects (void)
{
  return __builtin_constant_p (++gnu_constant_p_counter) +
         __builtin_constant_p (gnu_constant_p_counter = 1) +
         __builtin_constant_p (gnu_constant_p_runtime_value ());
}
