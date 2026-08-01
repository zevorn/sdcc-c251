struct __attribute__ ((__packed__)) gnu_prefix_attribute_struct
{
  unsigned char first;
  unsigned int second;
};

struct gnu_suffix_attribute_struct
{
  unsigned char value;
} __attribute__ ((__packed__, __aligned__ (1)));

__attribute__ ((__unused__)) static int gnu_prefix_attribute;
static __attribute__ ((__used__)) int gnu_infix_attribute;
static int gnu_suffix_attribute
  __attribute__ ((section ("GNU_)ATTRIBUTE"), aligned (sizeof (int))));

static int
gnu_attribute_function (int value __attribute__ ((unused)))
  __attribute__ ((noinline, format (printf, 1, 2)))
{
  return value + gnu_prefix_attribute + gnu_infix_attribute +
    gnu_suffix_attribute;
}

int
gnu_attribute_call (int value)
{
  return gnu_attribute_function (value);
}
