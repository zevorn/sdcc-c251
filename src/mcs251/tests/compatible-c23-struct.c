struct compatible_c23_struct
{
  int first;
  long second;
};

struct compatible_c23_struct
{
  int first;
  long second;
};

int
read_compatible_c23_struct (struct compatible_c23_struct *value)
{
  return value->first;
}
