unsigned char byte_value;
unsigned int large_counter;
volatile unsigned char large_dividend;
volatile unsigned char large_divisor;
volatile unsigned int large_remainder;

void
increment_byte (void)
{
    byte_value++;
}

void
add_two_bytes (void)
{
    byte_value += 2;
}

void
subtract_two_bytes (void)
{
    byte_value -= 2;
}

void
increment_large_counter (void)
{
    large_counter++;
}

void
remainder_large_bytes (void)
{
    large_remainder = large_dividend % large_divisor;
}

unsigned int
add16 (unsigned int left, unsigned int right)
{
    return left + right;
}

unsigned int
and16 (unsigned int left, unsigned int right)
{
    return left & right;
}

unsigned int
or16 (unsigned int left, unsigned int right)
{
    return left | right;
}

unsigned int
xor16 (unsigned int left, unsigned int right)
{
    return left ^ right;
}

unsigned long
xor32 (unsigned long left, unsigned long right)
{
    return left ^ right;
}

unsigned int
and16_literal (unsigned int value)
{
    return value & 0x5aa5u;
}

unsigned int
or16_literal (unsigned int value)
{
    return value | 0x5aa5u;
}

unsigned int
xor16_literal (unsigned int value)
{
    return value ^ 0x5aa5u;
}

unsigned char
less_than16_literal (unsigned int value)
{
    return value < 0x5aa5u;
}

unsigned int
increment16 (unsigned int value)
{
    return value + 1;
}

unsigned int
literal16 (void)
{
    return 0x1234;
}

unsigned int
load16 (unsigned int *pointer)
{
    return *pointer;
}

void
store16 (unsigned int *pointer, unsigned int value)
{
    *pointer = value;
}

unsigned char
stack_compound_literals (unsigned char value) __reentrant
{
    volatile unsigned char local = value;

    local &= 0x7f;
    local |= 0x01;
    local ^= 0x55;
    return local;
}
