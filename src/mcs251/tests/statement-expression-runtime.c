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
    int value = 3;
    unsigned char passed = 1;

    if (minimum_once (&value, 7) != 3 || value != 4)
        passed = 0;
    if (mixed_declarations (5) != 12)
        passed = 0;
    if (nested_expression (5) != 12)
        passed = 0;

    ({ ; });
    print_result (passed);
    for (;;)
        {
        }
}
