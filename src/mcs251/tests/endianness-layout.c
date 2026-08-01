/* Public object-layout probes for the MCS251 big-endian ABI. */

unsigned int mcs251_endian_word = 0x1234u;
unsigned long mcs251_endian_dword = 0x12345678ul;

unsigned char
mcs251_endian_word_byte (unsigned char index)
{
    return ((unsigned char *)&mcs251_endian_word)[index];
}

unsigned char
mcs251_endian_dword_byte (unsigned char index)
{
    return ((unsigned char *)&mcs251_endian_dword)[index];
}
