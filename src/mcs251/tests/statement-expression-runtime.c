__sfr __at (0x99) SBUF;

#define GNU_MIN(first, second) \
    ({ \
        __typeof__ (first) gnu_first = (first); \
        __typeof__ (second) gnu_second = (second); \
        gnu_first < gnu_second ? gnu_first : gnu_second; \
    })

unsigned char
__sdcc_external_startup (void)
{
    return 0;
}

static int
minimum_once (int *value, int limit)
{
    return GNU_MIN ((*value)++, limit);
}

static int
mixed_declarations (int value)
{
    return ({
        value++;
        int doubled = value * 2;

        doubled;
    });
}

static int
nested_expression (int value)
{
    return ({
        int inner = ({ value + 1; });

        inner * 2;
    });
}

struct heap_shape
{
    unsigned long end_chunk;
};

static inline unsigned long
heap_header_bytes (struct heap_shape *heap)
{
    return heap->end_chunk ? 8 : 4;
}

static inline unsigned long
heap_like_inline_cast (struct heap_shape *heap, unsigned long bytes)
{
    unsigned long chunks = bytes / 8UL;
    unsigned long oddments =
        ((bytes % 8UL) + heap_header_bytes (heap) + 7UL) / 8UL;

    return (unsigned long) ({
        __typeof__ (chunks + oddments) local_chunks = chunks + oddments;
        __typeof__ (heap->end_chunk) local_end = heap->end_chunk;

        local_chunks < local_end ? local_chunks : local_end;
    });
}

static void
print_result (unsigned char passed)
{
    if (passed)
        {
            SBUF = 'P';
            SBUF = 'A';
            SBUF = 'S';
            SBUF = 'S';
        }
    else
        {
            SBUF = 'F';
            SBUF = 'A';
            SBUF = 'I';
            SBUF = 'L';
        }
    SBUF = '\n';
}

void
main (void)
{
    struct heap_shape heap = {20};
    int value = 3;
    unsigned char passed = 1;

    if (minimum_once (&value, 7) != 3 || value != 4)
        passed = 0;
    if (mixed_declarations (5) != 12)
        passed = 0;
    if (nested_expression (5) != 12)
        passed = 0;
    if (heap_like_inline_cast (&heap, 17) != 4)
        passed = 0;
    heap.end_chunk = 0;
    if (heap_like_inline_cast (&heap, 17) != 0)
        passed = 0;

    ({ ; });
    print_result (passed);
    for (;;)
        {
        }
}
