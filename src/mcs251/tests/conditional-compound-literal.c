struct conditional_pair
{
  long long first;
  long second;
};

struct conditional_pair global_conditional_pair;

void
conditional_compound_literal_probe (void)
{
  global_conditional_pair = (struct conditional_pair) {
    0x7fffffffffffffffLL < 0LL ? 0LL : 0x7fffffffffffffffLL,
    999999999L
  };
}

long long
conditional_compound_literal_check (void)
{
  return global_conditional_pair.first + global_conditional_pair.second;
}
