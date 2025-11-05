// A lot of undefined behavior from C23 has been removed in C2y
// Martin Uecker's "Ghosts and Demons" gives a good overview.
// The numbering of the test cases below corresponds to the numbering therein.
// In many cases, the resolution was to make it a constraint violation.

#ifdef TEST1b
extern int a;
_Static_assert(1 || a); /* ERROR(SDCC) */
#endif

#ifdef TEST3
#define ASSEMBLE(uc1, uc2) uc1##uc2
int ASSEMBLE(\u00, c4); /* ERROR */
#endif

#ifdef TEST4a
char main(void); /* WARNING(SDCC) */
#endif

#ifdef TEST4b
long main(void); /* WARNING(SDCC) */
#endif

#ifdef TEST4b
void main(int); /* WARNING(SDCC) */
#endif

#ifdef TEST6
void f(void) {
int © rate = 100; /* ERROR */
}
#endif

#ifdef TEST7a
// Invalid multibyte character
#include <wchar.h>
const wchar_t wc = L'\U00110000'; /* WARNING */
const char *ws = "\U00110000"; /* WARNING */
#endif

#ifdef TEST8a
// Identifier declared with storage class extern while previous declaration with linkage is visible gets linkage of previous declaration,
// but the other way round is an error.

static int i;
extern int i;

extern int j; /* IGNORE */
static int j; /* ERROR */
#endif

#ifdef TEST8b
#pragma disable_warning 85
void g(void)
{
extern int x; /* IGNORE */
}
static int x; /* ERROR */
#endif

#ifdef TEST8c
#pragma disable_warning 85
// Block-scope a2 has internal linkage
static int a2;
void f2(void)
{
int b2;
{
extern int a2;
}
}

// Block-scope at has external linkage
static int a1; /* IGNORE */
void f1(void)
{
int a1;
{
extern int a1; /* ERROR */
}
}
#endif

#ifdef TEST19
struct f *p, *q;

void g1(void)
{
  *p; /* ERROR */
}

void g3(void)
{
  *q = *p; /* ERROR */
}
#endif

#ifdef TEST23
char foo_impl(int *p)
{
return p; /* WARNING */
}

char foo_expl(int *p)
{
return (char)p; /* WARNING */
}

_Bool foo_bool(int *p)
{
return (_Bool)p;
}
#endif

#ifdef TEST38a
struct A {
int a;
int x : (sizeof((struct A*)0)->a * 8); /* ERROR */
};
#endif

#ifdef TEST40
struct f { struct f *x; } *p;
void g(void)
{
(struct f)p; /* ERROR */
}
#endif

#ifdef TEST56
#pragma disable_warning 85
struct f;
void i(void)
{
struct f y; /* ERROR */
int i[]; /* ERROR */
}
#endif

#ifdef TEST57a
void i(void)
{
static void f(void); /* ERROR */
}
#endif

#ifdef TEST57b
void i(void)
{
register void f(void); /* ERROR */
}
#endif

// regarding struct/union with no named member: SDCC makes those without any member at all an error, but allow those with memebers, even if all are unnamed bit-fields.

#ifdef TEST58a
struct f { }; /* ERROR */
#endif

#ifdef TEST58b
union u { }; /* ERROR */
#endif

#ifdef TEST58c
struct g { struct { }; }; /* ERROR */
#endif

#ifdef TEST58d
struct h { int i:3; };
#endif

#ifdef TEST67
// extern inline declaration without definition in same translation unit
extern inline void f(void); /* ERROR */
#endif

#ifdef TEST75
int f(static void); /* ERROR */
int g(register void); /* ERROR */
int h(volatile void); /* ERROR */
int i(const void); /* ERROR */
int j(void x); /* ERROR */
int j(void x) { } /* ERROR */
#endif

#ifdef TEST87
struct foo;
static struct foo x; /* ERROR */
#endif

