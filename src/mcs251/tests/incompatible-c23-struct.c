struct incompatible_c23_struct
{
  int first;
};

struct incompatible_c23_struct
{
  int first;
  int second;
};

int
read_incompatible_c23_struct (struct incompatible_c23_struct *value)
{
  return value->first;
}
