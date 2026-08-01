__sfr __at (0x99) SBUF;

struct big_result
{
    unsigned char bytes[10];
};

static struct big_result source;
static struct big_result destination;

unsigned char
__sdcc_external_startup (void)
{
    return 0;
}

static struct big_result
make_big_result (unsigned char base)
{
    unsigned char index;

    for (index = 0; index < sizeof (source.bytes); ++index)
        source.bytes[index] = base + index;

    return source;
}

void
main (void)
{
    unsigned char passed;

    destination = make_big_result (0x30);
    passed = destination.bytes[0] == 0x30 &&
             destination.bytes[5] == 0x35 &&
             destination.bytes[9] == 0x39;

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

    for (;;)
        {
        }
}
