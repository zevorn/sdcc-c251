#include <stddef.h>

// Basic warnings
#ifdef TEST1
_Optional int *p;
int *q;

void f1(void)
{
	p = q;
}

void f2(void)
{
	q = p; /* WARNING */
}

void g(int *r, _Optional int *s);

void h(void)
{
	g (q, q);
	g (p, p); /* WARNING */
	g (p, q); /* WARNING */
	g (q, p);
}
#endif

// _Optional, unlike other standard qualifiers, has defined semantics when applied to functions.
#ifdef TEST2
typedef void f_t(void);
_Optional f_t *fp1;
f_t *fp2;

void f1(void)
{
	fp1 = fp2;
}

void f2(void)
{
	fp2 = fp1; /* WARNING */
}
#endif

// Test case from N3422
#ifdef TEST3
#pragma std_c2y
_Optional int *poi; // valid
typeof(&*poi) pi; // pi has type "int *" because of &
static_assert(_Generic(typeof(pi), int *: 1, default: 0));
#endif

// Test case from N3422
#ifdef TEST4
#pragma std_c2y
const int *pci; // valid
typeof(&*pci) pci2; // pci2 has type "const int *" despite &
static_assert(_Generic(typeof(pci2), const int *: 1, default: 0));
#endif

// Test case from N3422
#ifdef TEST5
int *_Optional opi; /* ERROR */ // invalid: "int *" is not a referenced type
_Optional int oi; /* ERROR */ // invalid: "int" is not a referenced type
_Optional struct s; /*todo! - shouldbe an error! */ // invalid: "struct s" is not a referenced type
_Optional int oa[2][3]; /* ERROR */ // invalid: int is not a referenced type
#endif

// Test case from N3422
#ifdef TEST6
_Optional int *poi; // valid
const int *pci; // valid
typeof(*poi) k; /* ERROR */ // invalid: "int" is not a referenced type
typeof(*pci) m; // valid
static_assert(_Generic(typeof(m), const int: 1, default: 0));
#endif

// Test case from N3422
#ifdef TEST7
typedef int U[15];
_Optional U oat; /* ERROR */ // invalid: oat has type "_Optional int [15]"
//todo - not an error! _Optional U *poat; // valid: poat has type "_Optional int (*)[15]"
#endif

// Test case from N3422
#ifdef TEST8
_Optional int *f(float); // valid
_Optional int f2(float); /* ERROR */ // invalid: int is not a referenced type
_Optional int (*fp)(float); /* ERROR */ // invalid: int is not a referenced type
#endif

#ifdef TEST9
typedef int F(float);
_Optional F *fp2; // valid: fp2 has type "int (*)(float) _Optional"
#endif

// Test case from N3422
#ifdef TEST10
void h(_Optional int); /* ERROR */ // invalid: "int" is not a referenced type
//todo - not an error? void l(_Optional int param[2][3]); // valid: param is a pointer
#endif

// Test case from N3422
#ifdef TEST11
#pragma disable_warning 84
void fred(_Optional int *i)
{
	void foo(int *);
	void bar(_Optional int *);
	int *j, k;
	_Optional int *m;
	j = i; /* WARNING */ // violates type constraints for =
	foo(i); /* WARNING */ // violates type constraints for function call

	m = i; // valid
	i = j; // valid
	bar(j); // valid
	j = (int *)i; // valid
	foo((int *)i); // valid
	// type constraints aren't lifted by path-sensitive analysis
	if (i) {
		j = i; /* WARNING */ // violates type constraints for =
		foo(i); /* WARNING */ // violates type constraints for function call
	}
	i = &k;
	j = i; /* WARNING */ // violates type constraints for =
	foo(i); /* WARNING */ // violates type constraints for function call
}
#endif

