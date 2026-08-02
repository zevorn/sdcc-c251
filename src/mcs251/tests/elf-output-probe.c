/* MCS-251 ELF-output probe: provides the two global functions the
   check-elf-output.py test looks for in the linked ELF symbol table. */

int elf_probe_value = 7;

int
elf_probe_add (int left, int right)
{
    return left + right;
}

int
elf_probe_main (void)
{
    return elf_probe_add (elf_probe_value, 1);
}