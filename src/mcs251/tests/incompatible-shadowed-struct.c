struct shadowed_external_record
{
  int value;
};

extern struct shadowed_external_record shadowed_external_record;

void
declare_shadowed_external_record (void)
{
  struct shadowed_external_record
  {
    int value;
  };
  extern struct shadowed_external_record shadowed_external_record;
}
