extern int compatible_external_array[];
extern int compatible_external_array[2];

int
read_compatible_external_array (void)
{
  return compatible_external_array[0];
}
