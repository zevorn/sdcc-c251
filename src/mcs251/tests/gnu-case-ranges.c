/* GNU case-range expressions (`case A ... B:`) are accepted in GNU modes.
   They are a C2y-standard feature in ISO C, so strict C11/C17 reject them. */

enum
{
  GNU_RANGE_NONE = 0,
  GNU_RANGE_SMALL = 1,
  GNU_RANGE_MEDIUM = 2,
  GNU_RANGE_LARGE = 3
};

unsigned char
mcs251_gnu_case_range (unsigned char value)
{
  switch (value)
    {
    case 0:
      return GNU_RANGE_NONE;
    case 1 ... 3:
      return GNU_RANGE_SMALL;
    case 4 ... 8:
      return GNU_RANGE_MEDIUM;
    case 9 ... 16:
      return GNU_RANGE_LARGE;
    default:
      return 4;
    }
}