struct empty_struct
{
};

union empty_union
{
};

struct empty_wrapper
{
  struct empty_struct prefix;
  unsigned char payload;
};

struct empty_struct empty_struct_object;
union empty_union empty_union_object;
struct empty_wrapper empty_wrapper_object;

/* GNU-style initializers on a size-0 (empty) struct/union: emitRegularMap
 * -> printIvalStruct() previously dereferenced the NULL field list and
 * SIGSEGV'd (model-large, mailbox.c).  These globals with = {} initializers
 * must now compile without crashing. */
struct empty_struct empty_struct_object2 = {};
union empty_union empty_union_object2 = {};


unsigned char empty_struct_size_is_zero
  [sizeof (struct empty_struct) == 0 ? 1 : -1];
unsigned char empty_union_size_is_zero
  [sizeof (union empty_union) == 0 ? 1 : -1];

int
gnu_empty_aggregate_probe (void)
{
  struct empty_struct local;
  struct empty_struct *pointer = &empty_struct_object;

  return sizeof (local) + empty_wrapper_object.payload +
    (pointer + 1 == pointer);
}
