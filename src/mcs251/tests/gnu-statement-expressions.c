typedef _Bool gnu_bool;

#define GNU_MIN(first, second) \
  ({ \
    __typeof__ (first) gnu_first = (first); \
    __typeof__ (second) gnu_second = (second); \
    gnu_first < gnu_second ? gnu_first : gnu_second; \
  })

int
gnu_statement_expression_min (int first, int second)
{
  return GNU_MIN (first, second);
}

int
gnu_statement_expression_once (int *value)
{
  return ({
    int original = (*value)++;

    original + 1;
  });
}

int
gnu_statement_expression_mixed (int value)
{
  return ({
    value++;
    int doubled = value * 2;

    doubled;
  });
}

int
gnu_statement_expression_nested (int value)
{
  return ({
    int inner = ({ value + 1; });

    inner * 2;
  });
}

static inline unsigned int
gnu_statement_expression_cast_inline (unsigned int first,
                                      unsigned int second)
{
  return (unsigned int) ({
    __typeof__ (first) local_first = first;
    __typeof__ (second) local_second = second;

    local_first < local_second ? local_first : local_second;
  });
}

unsigned int
gnu_statement_expression_redundant_cast (unsigned int first,
                                         unsigned int second)
{
  return gnu_statement_expression_cast_inline (first, second);
}

struct gnu_statement_expression_heap
{
  unsigned long end_chunk;
};

typedef unsigned long gnu_statement_expression_chunksz_t;

static inline unsigned long
gnu_statement_expression_header_bytes (
  struct gnu_statement_expression_heap *heap)
{
  return heap->end_chunk ? 8 : 4;
}

static inline gnu_statement_expression_chunksz_t
gnu_statement_expression_inline_cast (
  struct gnu_statement_expression_heap *heap,
  unsigned long bytes, unsigned long extra)
{
  unsigned long chunks = bytes / 8UL + extra / 8UL;
  unsigned long oddments =
    ((bytes % 8UL) + (extra % 8UL) +
     gnu_statement_expression_header_bytes (heap) + 7UL) / 8UL;

  return (gnu_statement_expression_chunksz_t) ({
    __typeof__ (chunks + oddments + 0) local_chunks =
      chunks + oddments + 0;
    __typeof__ (heap->end_chunk) local_end = heap->end_chunk;

    local_chunks < local_end ? local_chunks : local_end;
  });
}

unsigned long
gnu_statement_expression_inline_cast_user (
  struct gnu_statement_expression_heap *heap, unsigned long bytes)
{
  gnu_statement_expression_chunksz_t chunk_size =
    gnu_statement_expression_inline_cast (heap, bytes, 0);

  if (!chunk_size)
    return 0;
  return chunk_size;
}

gnu_bool
gnu_statement_expression_boolean (int value)
{
  return ({
    gnu_bool result = value != 0;

    result;
  });
}

void
gnu_statement_expression_void (int *value)
{
  (void) ({
    (*value)++;
    (void) 0;
  });
}

void
gnu_statement_expression_empty (void)
{
  ({ ; });
}
