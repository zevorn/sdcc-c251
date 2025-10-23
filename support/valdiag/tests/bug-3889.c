/* bug-3889.c

   Incomplete check of block-scope extern declaration vs. file-scope declaration.
 */

#ifdef TEST1
char c; /* IGNORE */
void f(void)
{
extern int c; /* ERROR */
}
#endif

