typedef int gnu_int_function (int);
typedef int gnu_long_function (long);
typedef int gnu_char_function (char);
typedef int gnu_short_function (short);
typedef int gnu_float_function (float);
typedef int gnu_char_pointer_function (char *);
typedef int gnu_array_parameter_function (int[4]);
typedef int gnu_pointer_parameter_function (int *);
typedef int gnu_const_array_parameter_function (const int[4]);
typedef int gnu_variadic_function (int, ...);
typedef int gnu_unprototyped_function ();
typedef int (*gnu_int_function_pointer) (int);
typedef int (*gnu_long_function_pointer) (long);
typedef int *gnu_int_pointer;
typedef int *const gnu_const_int_pointer;
typedef const int gnu_const_complete_array[4];

extern int gnu_complete_array[4];
extern int gnu_incomplete_array[];
static int gnu_stored_int;

struct gnu_first_record
{
  int value;
};

struct gnu_second_record
{
  int value;
};

struct gnu_array_record
{
  int values[4];
};

enum gnu_first_enum
{
  GNU_FIRST_VALUE
};

enum gnu_second_enum
{
  GNU_SECOND_VALUE
};

typedef int gnu_enum_function (enum gnu_first_enum);

_Static_assert (
  __builtin_types_compatible_p (int, const int),
  "top-level qualifiers do not affect type compatibility");
_Static_assert (
  __builtin_types_compatible_p (__typeof__ (gnu_complete_array),
                                __typeof__ (gnu_incomplete_array)),
  "complete and incomplete arrays of the same element type are compatible");
_Static_assert (
  __builtin_types_compatible_p (__typeof__ (gnu_complete_array),
                                gnu_const_complete_array),
  "top-level array qualifiers do not affect compatibility");
_Static_assert (
  __builtin_types_compatible_p (__typeof__ ((int)0), int),
  "typeof results participate in type compatibility checks");
_Static_assert (
  __builtin_types_compatible_p (__typeof__ (gnu_stored_int), int),
  "object storage does not participate in type compatibility checks");
_Static_assert (
  __builtin_types_compatible_p (gnu_int_pointer,
                                gnu_const_int_pointer),
  "top-level pointer qualifiers do not affect compatibility");
_Static_assert (
  __builtin_types_compatible_p (gnu_int_function,
                                gnu_int_function),
  "identical function types are compatible");
_Static_assert (
  __builtin_types_compatible_p (gnu_unprototyped_function,
                                gnu_int_function),
  "an int parameter is unchanged by default argument promotions");
_Static_assert (
  __builtin_types_compatible_p (gnu_unprototyped_function,
                                gnu_long_function),
  "a long parameter is unchanged by default argument promotions");
_Static_assert (
  __builtin_types_compatible_p (gnu_unprototyped_function,
                                gnu_char_pointer_function),
  "a pointer parameter is unchanged by default argument promotions");
_Static_assert (
  __builtin_types_compatible_p (gnu_array_parameter_function,
                                gnu_pointer_parameter_function),
  "array parameters are adjusted to pointer types");
_Static_assert (
  __builtin_types_compatible_p (enum gnu_first_enum,
                                enum gnu_first_enum),
  "an enumerated type is compatible with itself");
_Static_assert (
  __builtin_types_compatible_p (enum gnu_first_enum, unsigned char),
  "an enumerated type is compatible with its implementation type");

_Static_assert (
  !__builtin_types_compatible_p (int, unsigned int),
  "signed and unsigned integer types are incompatible");
_Static_assert (
  !__builtin_types_compatible_p (int *, const int *),
  "pointed-to qualifiers affect type compatibility");
_Static_assert (
  !__builtin_types_compatible_p (__typeof__ (gnu_complete_array), int *),
  "array and pointer types are incompatible");
_Static_assert (
  !__builtin_types_compatible_p (int[4], int[5]),
  "arrays with distinct constant bounds are incompatible");
_Static_assert (
  !__builtin_types_compatible_p (struct gnu_first_record,
                                 struct gnu_second_record),
  "distinct structure types are incompatible");
_Static_assert (
  !__builtin_types_compatible_p (gnu_int_function,
                                 gnu_long_function),
  "different function parameter types are incompatible");
_Static_assert (
  !__builtin_types_compatible_p (gnu_int_function,
                                 gnu_variadic_function),
  "fixed and variadic function types are incompatible");
_Static_assert (
  !__builtin_types_compatible_p (gnu_array_parameter_function,
                                 gnu_const_array_parameter_function),
  "pointed-to qualifiers remain after array parameter adjustment");
_Static_assert (
  !__builtin_types_compatible_p (gnu_unprototyped_function,
                                 gnu_char_function),
  "default argument promotions change a char parameter");
_Static_assert (
  !__builtin_types_compatible_p (gnu_unprototyped_function,
                                 gnu_short_function),
  "default argument promotions change a short parameter");
_Static_assert (
  !__builtin_types_compatible_p (gnu_unprototyped_function,
                                 gnu_float_function),
  "default argument promotions change a float parameter");
_Static_assert (
  !__builtin_types_compatible_p (gnu_unprototyped_function,
                                 gnu_enum_function),
  "default argument promotions change a narrow enum parameter");
_Static_assert (
  !__builtin_types_compatible_p (gnu_int_function_pointer,
                                 gnu_long_function_pointer),
  "function pointer parameter types participate in compatibility checks");
_Static_assert (
  !__builtin_types_compatible_p (enum gnu_first_enum,
                                 enum gnu_second_enum),
  "distinct enumerated types are incompatible");

static int gnu_types_compatible_array[4];

#define GNU_ZERO_OR_COMPILE_ERROR(condition) \
  ((int) sizeof (char[1 - (2 * !(condition))]) - 1)
#define GNU_IS_ARRAY(array) \
  GNU_ZERO_OR_COMPILE_ERROR ( \
    !__builtin_types_compatible_p (__typeof__ (array), \
                                   __typeof__ (&(array)[0])))
#define GNU_ARRAY_SIZE(array) \
  (GNU_IS_ARRAY (array) + (sizeof (array) / sizeof ((array)[0])))

int
gnu_types_compatible_probe (void)
{
  return !__builtin_types_compatible_p (
    __typeof__ (gnu_types_compatible_array),
    __typeof__ (&gnu_types_compatible_array[0]));
}

int
gnu_member_array_probe (struct gnu_array_record *record)
{
  return GNU_ARRAY_SIZE (record->values);
}
