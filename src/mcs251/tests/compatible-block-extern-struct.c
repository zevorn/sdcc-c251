struct block_external_record
{
  int value;
};

extern struct block_external_record block_external_records[];

int
read_block_external_record (void)
{
  extern struct block_external_record block_external_records[];
  extern struct block_external_record block_external_records[];

  return block_external_records[0].value;
}
