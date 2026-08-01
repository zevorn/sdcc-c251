signed int
mcs251_extend_signed_char (signed char value)
{
    return value;
}

unsigned int
mcs251_extend_unsigned_char (unsigned char value)
{
    return value;
}

signed int
mcs251_extend_signed_add (signed char value, signed int addend)
{
    return value + addend;
}

unsigned int
mcs251_extend_unsigned_add (unsigned char value, unsigned int addend)
{
    return value + addend;
}

unsigned long
mcs251_replace_low_word (unsigned long value)
{
    return (value & 0xffff0000ul) | 0x5678ul;
}

unsigned long
mcs251_replace_high_word (unsigned long value)
{
    return (value & 0x0000fffful) | 0x56780000ul;
}
