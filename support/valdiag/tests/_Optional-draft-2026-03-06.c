// Test diagnostics for _Optional. Based on examples from _Optional TS draft from 2026-03-06.

#pragma std_c23

#include <assert.h>

#ifdef TEST1
#include <stdio.h>

struct S {
  char m[64];
};

char *str_from_struct(_Optional const struct S *pocs)
{
  static char s[] = "";
  if (!pocs) return s;

  // valid: array to pointer decay removes _Optional
  puts(pocs->m);

  static_assert(_Generic(pocs->m,
                         const char *: 1,
                         default: 0));

  // invalid: array to pointer decay does not remove const
  return pocs->m; /* IGNORE */ // todo: implement warning!
}

char *str_from_array(_Optional const char (*paocc)[64])
{
  static char s[] = "";

  if (!paocc) return s;

  // valid: array to pointer decay removes _Optional
  puts(*paocc);

  static_assert(_Generic(*paocc,
                         const char *: 1,
                         default: 0)); /* IGNORE */ // TODO: implement!

  // invalid: array to pointer decay does not remove const
  return *paocc; /* IGNORE */ // todo: implement warning!
}
#endif

#ifdef TEST2
typedef int RoundFn(double); /* IGNORE */ // We only care about diagnostics for _Optional, not double.
RoundFn towardzero;

RoundFn *get_round_fn(_Optional RoundFn *pof)
{
  if (!pof) return &towardzero;

  // valid: function to pointer decay removes _Optional
  static_assert(_Generic(*pof, RoundFn *: 1, default: 0));
  return *pof;
}
#endif

#ifdef TEST3
_Optional int *poi;

// fails: qualifier is dropped from controlling expression
static_assert(_Generic(*poi,
                       _Optional int: 1,
                       default: 0)); /* ERROR */
#endif

#ifdef TEST4
int autumn(_Optional int *poi)
{
  return poi[0]; // recommended diagnostic /* WARNING */
}

int *brazil(_Optional int *poi)
{
  return &poi[0]; // recommended diagnostic /* WARNING */
}
#endif

#ifdef TEST5
int *foxton(_Optional int *poi)
{
  static typeof_unqual(poi[0]) i; // no recommended diagnostic
  return &i;
}
#endif

#ifdef TEST6
#if 0 // BUG
void anna(_Optional typeof(void (void)) *pof)
{
  pof(); // recommended diagnostic /* IGNORE */ // BUG, should be warning!
}
#endif
#endif

#ifdef TEST7
#if 0 // BUG
int *barton(_Optional typeof(int (void)) *pof)
{
  static typeof(pof()) retval;
  return &retval;
}
#endif
#endif

#ifdef TEST8
struct S {
  int m;
};

int arabella(_Optional struct S *pos)
{
  return pos->m; // recommended diagnostic /* IGNORE */ // BUG missing warning
}

int *albion(_Optional struct S *pos)
{
  return &pos->m; // recommended diagnostic /* IGNORE */ // BUG missing warning
}
#endif

#ifdef TEST9
struct S {
  int m;
};

char *elsworth(_Optional struct S *pos)
{
  static char mbytes[sizeof(pos->m)]; //no recommended diagnostic
  return mbytes;
}
#endif

#ifdef TEST10
int *electron(_Optional int *poi)
{
  return poi++; // recommended diagnostic /* WARNING */
}

int *aberdeen(_Optional int *poi)
{
  return poi--; // recommended diagnostic /* WARNING */
}
#endif

#ifdef TEST11
char *newmarket(_Optional int *poi)
{
  static char ibytes[sizeof(poi--)]; // no recommended diagnostic
  return ibytes;
}
#endif

#ifdef TEST12
void atom(void)
{
  // valid: referenced type is qualified
  _Optional int *poi = (_Optional int *){nullptr};

  // invalid: qualified type is not a referenced type
  poi = &(_Optional int){0}; /* WARNING */
  (_Optional int){0}; /* WARNING */
  (int *_Optional){nullptr}; /* WARNING */
}
#endif

#ifdef TEST13
int *stork(_Optional int *poi)
{
  return ++poi; // recommended diagnostic /* WARNING */
}

int *phoenix(_Optional int *poi)
{
  return --poi; // recommended diagnostic /* WARNING */
}
#endif

#ifdef TEST14
char *cambridge(_Optional int *poi)
{
  static char ibytes[sizeof(++poi)]; // no recommended diagnostic
  return ibytes;
}
#endif

#ifdef TEST15
#include <string.h>
int opt_strcmp(_Optional const char *s1,
               _Optional const char *s2)
{
  if (!s1) s1 = "";
  if (!s2) s2 = "";

  static_assert(_Generic(&*s1,
                const char *: 1,
                default : 0));

  return strcmp(&*s1, &*s2);
}
#endif

#ifdef TEST16
void hawk(_Optional int *poi)
{
  *poi = 10; // recommended diagnostic /* WARNING */
}

int heron(_Optional int *poi)
{
  return *poi; // recommended diagnostic /* WARNING */
}

int *roadrunner(_Optional int *poi)
{
  return &*poi; // recommended diagnostic /* WARNING */
}
#endif

#ifdef TEST17
#if 0 // Test uses stream fetures not supported by SDCC library.
#include <stdio.h>

int opt_fflush(_Optional FILE *stream)
{
  return fflush((FILE *)stream);
}

#define FLUSH_ONE(STREAM) opt_fflush(&*(STREAM))
#define FLUSH_ALL()       opt_fflush(nullptr)
 
int main(void)
{
  _Optional FILE *stream = fopen("test", "wb");
  FLUSH_ONE(stream); // recommended diagnostic
  return 0;
}
#endif
#endif

#ifdef TEST18
void fulbourn(_Optional int *poi)
{
  static_assert(_Generic(*poi, int: 1, default: 0));
}
#endif

#ifdef TEST19
static_assert(alignof(_Optional int) == alignof(int));
static_assert(sizeof(_Optional int) == sizeof(int));
#endif

#ifdef TEST20
#if 0 // Test usees getenv not present in SDCC library.
#include <stdlib.h>
#define ALIGNED_ALLOC(T) \
  (T *)aligned_alloc(alignof(T), sizeof(T))

void falcon(void)
{
  auto pci = ALIGNED_ALLOC(const int); // valid
  *pci = 0; // constraint violation (lvalue is not modifiable)

  auto poc = ALIGNED_ALLOC(_Optional char); // valid
  getenv(poc); // violates type constraints for call

  // violates type constraints for =
  double *pd = ALIGNED_ALLOC(int);
}
#endif
#endif

#ifdef TEST21
// valid: redundant casts convert to type "int"
int i = (_Optional int)0;
static_assert(_Generic((_Optional int)0,
                       int: 1, default: 0));

// valid: cast discards the fractional part
double d = (_Optional int)3.1; /* IGNORE */ // We only care about diagnostics for _Optional, not double.
#endif

#ifdef TEST22
int medusa(_Optional const int *poi)
{
  return *(int *)poi; // no recommended diagnostic
}
#endif

#ifdef TEST23
#include <stdio.h>

void purple(_Optional char *poi)
{
  if (!poi) return;

  // passes: + operator removes _Optional
  static_assert(_Generic(poi + 1,
                         char *: 1,
                         default: 0));
  puts(poi + 1);

  // passes: - operator removes _Optional
  static_assert(_Generic(poi - 1,
                         char *: 1,
                         default: 0));
  puts(poi - 1);
}
#endif

#ifdef TEST24
#include <stddef.h>

int *amber(_Optional int *poi)
{
  return poi + 1; // recommended diagnostic /* WARNING */
}

ptrdiff_t blue(_Optional int *poi, int *pi)
{
  return poi - pi; // recommended diagnostic /* WARNING */
}

int *green(_Optional int *poi)
{
  return poi - 0; // recommended diagnostic /* IGNORE */ // BUG: missing warning
}

int *black(_Optional int *poi)
{
  return 0 + poi; // recommended diagnostic /* WARNING */
}
#endif

#ifdef TEST25
char *caldecote(_Optional int *poi)
{
  static char ibytes[sizeof(poi+1)]; // no recommended diagnostic
  return ibytes;
}
#endif

#ifdef TEST26
int is_lt(_Optional int *poi, int *pi)
{
  return poi < pi; // recommended diagnostic /* WARNING */
}

int is_le(_Optional int *poi, int *pi)
{
  return poi <= pi; // recommended diagnostic /* WARNING */
}

int is_ge(_Optional int *poi, int *pi)
{
  return poi >= pi; // recommended diagnostic /* WARNING */
}

int is_gt(_Optional int *poi, int *pi)
{
  return poi > pi; // recommended diagnostic /* WARNING */
}
#endif

#ifdef TEST27
int *coton(_Optional int *poi, int *pi)
{
  static typeof(poi < pi) result; // no recommended diagnostic
  return &result;
}
#endif

#ifdef TEST28
int *spider(const int *pci)
{
  static int i, *pi; // the object i is modifiable

  if (pci == &i) {   // pci points to i within the block
    pi = pci;        // violates type constraints for = /* WARNING */
    pi = (int *)pci; // valid
  }

  pci = &i;          // pci points to i thereafter
  pi = pci;          // violates type constraints for = /* WARNING */
  return &i;         // address of i escapes
}
#endif

#ifdef TEST29
void fox(_Optional int *poi)
{
  int i, *pi;

  if (poi != nullptr) { // poi is non-null within the block /* IGNORE */ // Check: why evelyn warning here?
    pi = poi;           // violates type constraints for = /* WARNING */
    pi = (int *)poi;    // valid
  }

  poi = &i;             // poi is non-null thereafter
  pi = poi;             // violates type constraints for = /* WARNING */
}
#endif

#ifdef TEST30
typeof(_Optional int) *poi;
static_assert(_Generic(poi,
                       _Optional int *: 1,
                       default: 0));

typeof_unqual(_Optional int) *pi;
static_assert(_Generic(pi, int *: 1, default: 0));
#endif

#ifdef TEST31
#if 0 // Incomplete support for _Optional for function types.
#define DECL_PTR(T, N) \
  static typeof (T) *N; \
  void set_##N (typeof (T) *arg_##N) { N = arg_##N; } \
  typeof (T) *get_##N (void) { return N; }

DECL_PTR(int (double), round) // type "int (*)(double)"
DECL_PTR(_Optional int, opt_limit) // type "_Optional int *"
#endif
#endif

#ifdef TEST32
#if 0 // Incomplete support for _Optional for function types.
// valid: referenced type is qualified
_Optional typeof(int (float)) *pofri;
_Optional typeof_unqual(int (float)) *pofri2;

// invalid: qualified type is not a referenced type
_Optional typeof(int (float)) ofri;
_Optional typeof_unqual(int (float)) ofri2;
#endif
#endif

#ifdef TEST33
#include <stdlib.h>

// & yields a pointer to non-optional-qualified type
#define optional_cast(p) ((typeof(&*(p)))(p))

// const-qualified referenced type of s is accidental
void free_str_1(_Optional const char *s)
{
  free((char *)s); // undefined behavior if *s is a const object
}

// const-qualified referenced type of s is accidental
void free_str_2(_Optional const char *s)
{
  free(optional_cast(s)); // constraint violation /* IGNORE */ BUG: missing diagnostic
}

// s has the intended type
void free_str_3(_Optional char *s)
{
  free(optional_cast(s)); // valid: no action if s is null
}
#endif

#ifdef TEST34
const int ci; // valid
restrict int ri; // constraint violation /* ERROR */
volatile int vi; // valid
#ifndef __STDC_NO_ATOMICS__
_Atomic int ai; // valid
#endif
_Optional int oi; // constraint violation /* WARNING */
#endif

#ifdef TEST35
#undef NDEBUG
#include <assert.h>

void proton(int *pi)
{
  assert(pi != nullptr);
  (*pi)++;
}
#endif

#ifdef TEST36
typedef void TF(void);
TF func;

void neutron(void)
{
  _Optional TF *pof;
  TF *pf;

  pof = nullptr; // valid
  pof = &func;   // valid
  pf = nullptr;  // valid
  pf = pof;      // constraint violation /* WARNING */
}
#endif

#ifdef TEST37
void pisces(const bool do_store)
{
  int i;
  _Optional int *poi = nullptr;

  if (do_store)
    poi = &i;

  if (do_store)
    *poi = 1; // possible diagnostic /* IGNORE */
}
#endif

#ifdef TEST38
void phileas(_Optional int *const poi)
{
  bool do_store = true;

  if (!poi)
    do_store = false;

  if (do_store)
    *poi = 1; // possible diagnostic /* IGNORE */
}
#endif

#ifdef TEST39
void fred(_Optional int *poi)
{
  int i;
  _Optional int *poi_2 = &i;

  for (i = 0; i < 200; ++i)
    poi_2 = (i * 13) % 2 ? &i : poi;

  *poi_2 = 1; // possible diagnostic /* IGNORE */
}
#endif

#ifdef TEST40
void ram(int *);
void jim(_Optional int *poi)
{
  int i[16];

  poi = i;       // constrains poi to non-null
  *poi = 5;      // no recommended diagnostic
  ram(&*poi);    // no recommended diagnostic
  ram(&poi[15]); // no recommended diagnostic
}
#endif

#ifdef TEST41
void hw(int *);

int sheila(_Optional int *poi)
{
  // constrains poi to non-null in the secondary block
  if (poi) {
    *poi = 5;     // no recommended diagnostic
    hw(&*poi);    // no recommended diagnostic
    hw(&poi[15]); // no recommended diagnostic
  }

  // constrains poi to non-null in the secondary block
  for (; poi;) {
    *poi = 6;     // no recommended diagnostic
    hw(&*poi);    // no recommended diagnostic
    hw(&poi[15]); // no recommended diagnostic
    break;
  }

  // constrains poi to non-null in the secondary block
  while (poi) {
    *poi = 7;     // no recommended diagnostic
    hw(&*poi);    // no recommended diagnostic
    hw(&poi[15]); // no recommended diagnostic
    break;
  }

  // constrains poi to non-null in the secondary block
  if (!poi) {
  } else {
    *poi = 8;     // no recommended diagnostic
    hw(&*poi);    // no recommended diagnostic
    hw(&poi[15]); // no recommended diagnostic
  }

  /* constrains poi to non-null during evaluation of the
  second operand */
  return poi ? *poi : 0; // no recommended diagnostic
}
#endif

#ifdef TEST42
static void fs(_Optional int *poi)
{
  *poi = 10; // possible diagnostic /* IGNORE */
}
void hazel(void)
{
  int i;
  fs(&i);
}
#endif

#ifdef TEST43
static _Optional int *vdu(void)
{
  static int i;
  return &i;
}
void lynne(void)
{
  _Optional int *poi;
  poi = vdu();
  *poi = 2; // possible diagnostic /* IGNORE */
}
#endif

#ifdef TEST44
void andy(_Optional int *poi)
{
  int *pi;
  
  pi = (int *)poi;          // does not constrain pi to non-null
  *(_Optional int *)pi = 2; // recommended diagnostic /* WARNING */
}
#endif

#ifdef TEST45
void tina(int *pi)
{
  _Optional int *poi;

  poi = pi;  // does not constrain poi to non-null
  *poi = 10; // recommended diagnostic /* WARNING */
}
#endif

#ifdef TEST46
int avon(int *pi)
{
  _Optional int *poi;

  // constrains pi to non-null on the fallthrough path
  if (!pi) return 0;

  poi = pi;    // non-null constraint is copied from pi to poi
  return *poi; // no recommended diagnostic
}
#endif

#ifdef TEST47
_Optional int *volatile poi;
int brisbane(void)
{
  // does not constrain poi to non-null on either path
  if (!poi) return 0;

  return *poi; // recommended diagnostic /* WARNING */
}
#endif

#ifdef TEST48
_Optional int *poi;
void perth(_Optional int **ppoi)
{
  // constrains poi to non-null on the fallthrough path
  if (!poi) return;

  *poi = 1;        // no recommended diagnostic
  *ppoi = nullptr; /* analysis discards non-null constraint
                      on poi because *ppoi could alias poi */
  *poi = 2;        // recommended diagnostic /* WARNING */
}
#endif

#ifdef TEST49
void victoria(_Optional int **ppoi_1, _Optional int **ppoi_2)
{
  // constrains *ppoi_1 to non-null on the fallthrough path
  if (!*ppoi_1) return;

  **ppoi_1 = 1;      // no recommended diagnostic
  *ppoi_2 = nullptr; /* analysis discards non-null constraint
                        on *ppoi_1 because *ppoi_2 could
                        alias *ppoi_1 */
  **ppoi_1 = 2;      // recommended diagnostic /* WARNING */
}
#endif

#ifdef TEST50
_Optional int *poi;
void darwin(_Optional int **ppoi, _Optional int *upoi)
{
  // constrains poi to non-null on the fallthrough path
  if (!poi) return;

  *poi = 1;     // no recommended diagnostic
  *ppoi = upoi; /* analysis discards non-null constraint
                   on poi because *ppoi could alias poi */
  *poi = 2;     // recommended diagnostic /* WARNING */
}
#endif

#ifdef TEST51
void jordan(_Optional int *poi)
{
  _Optional int *poi_2, **ppoi = &poi_2;

  // constrains poi to non-null on the fallthrough path
  if (!poi) return;

  *poi = 1;        // no recommended diagnostic
  *ppoi = nullptr; /* non-null constraint on poi is unaffected
                      because *ppoi does not alias poi */
  *poi = 2;        // no recommended diagnostic
}
#endif

#ifdef TEST52
void orlando(_Optional int **ppoi_1,
             _Optional int ** restrict ppoi_2)
{
  // constrains *ppoi_1 to non-null on the fallthrough path
  if (!*ppoi_1) return;

  **ppoi_1 = 1;      // no recommended diagnostic
  *ppoi_2 = nullptr; /* non-null constraint on *ppoi_1 is
                        unaffected because *ppoi_2 cannot
                        alias *ppoi_1 */
  **ppoi_1 = 2;      // no recommended diagnostic /* IGNORE */ TODO: improve analysis
}
#endif

#ifdef TEST53
_Optional int *poi;
void buxton(void); // has unknown side effects
void adelaide(void)
{
  // constrains poi to non-null on the fallthrough path
  if (!poi) return;

  *poi = 1; // no recommended diagnostic /* IGNORE */ TODO: improve analysis for global variables.
  buxton(); /* analysis discards non-null constraint on poi
               because buxton could modify poi */
  *poi = 2; // recommended diagnostic /* WARNING */
}
#endif

#ifdef TEST54
void bethany(void); // has unknown side effects
void lazarus(_Optional int **ppoi)
{
  // constrains *ppoi to non-null on the fallthrough path
  if (!*ppoi) return;

  **ppoi = 1; // no recommended diagnostic
  bethany();  /* analysis discards non-null constraint on *ppoi
                 because bethany could modify *ppoi */
  **ppoi = 2; // recommended diagnostic /* WARNING */
}
#endif

#ifdef TEST55
void morris(_Optional int **ppoi); // has unknown side effects
void aquarius(_Optional int *poi)
{
  // constrains poi to non-null on the fallthrough path
  if (!poi) return;

  *poi = 1;     // no recommended diagnostic /* IGNORE */ TODO: improve analysis
  morris(&poi); /* analysis discards non-null constraint on poi
                   because morris could modify poi */
  *poi = 2;     // recommended diagnostic /* WARNING */
}
#endif

#ifdef TEST56
void omega(_Optional int *const *ppoi); // unknown side effects
_Optional int **ppoi;
void spinner(_Optional int *poi)
{
  // constrains poi to non-null on the fallthrough path
  if (!poi) return;

  *poi = 1;    // no recommended diagnostic /* IGNORE */ TODO: improve analysis
  omega(&poi); // non-null constraint on poi is unaffected
  *poi = 2;    // no recommended diagnostic /* IGNORE */ TODO: improve analysis
  ppoi = &poi; // address of poi escapes this function
  omega(&poi); /* analysis discards constraint on poi because
                  omega could modify *ppoi (aka poi) */
  *poi = 3;    // recommended diagnostic /* WARNING */
}
#endif

#ifdef TEST57
_Optional int *poi;
void chandler(_Optional int **ppoi)
{
  int i;

  // constrains poi to non-null on the fallthrough path
  if (!poi) return;

  *poi = 1;   // no recommended diagnostic /* IGNORE */ TODO: improve analysis
  *ppoi = &i; // non-null constraint on poi is unaffected
  *poi = 2;   // no recommended diagnostic /* IGNORE */ TODO: improve analysis
}
#endif

#ifdef TEST58
_Optional int *poi;
void monica(_Optional int **ppoi, _Optional int *lpoi)
{
  // constrains poi and lpoi to non-null on the fallthrough path
  if (!poi || !lpoi) return;

  *poi = 1;     // no recommended diagnostic /* IGNORE */ TODO: improve analysis
  *ppoi = lpoi; // non-null constraint on poi is unaffected
  *poi = 2;     // no recommended diagnostic /* IGNORE */ TODO: improve analysis
}
#endif

#ifdef TEST59
void rachel(_Optional int **ppoi_1, _Optional int **ppoi_2)
{
  int i;

  // constrains *ppoi_1 to non-null on the fallthrough path
  if (!*ppoi_1) return;

  **ppoi_1 = 1; // no recommended diagnostic
  *ppoi_2 = &i; // non-null constraint on *ppoi_1 is unaffected
  **ppoi_1 = 2; // no recommended diagnostic /* IGNORE */ TODO: improve analysis?
}
#endif

#ifdef TEST60
_Optional int *poi;
void ursula(void) [[reproducible]]; // no observable effects /* IGNORE */ TODO: implement the attribute
void phoebe(void)
{
  // constrains poi to non-null on the fallthrough path
  if (!poi) return;

  *poi = 1; // no recommended diagnostic /* IGNORE */ TODO: improve analysis
  ursula(); // non-null constraint on poi is unaffected
  *poi = 2; // no recommended diagnostic /* IGNORE */ TODO: improve analysis
}
#endif

#ifdef TEST61
_Optional int *ptr_to_optional; // valid
int *_Optional optional_ptr; // constraint violation /* ERROR */
#endif

#ifdef TEST62
// declaration constraint does not apply to typedef
typedef int TAI[2][3];
typedef _Optional int TOI;
typedef TOI TAOI[2][3];
typedef _Optional TAI TOAI;

// valid: referenced type is qualified
TAOI *paoi; // pointer to array of optional int
TOI (*paoi)[2][3]; // as above
_Optional int (*paoi)[2][3]; // as above
TOAI *poai; // pointer to optional array of int
_Optional TAI *poai; // as above

// effect of qualifying array or element type is the same
static_assert(_Generic(paoi, typeof(poai): 1, default: 0));

// invalid: qualified type is not a referenced type
TOAI oai; // optional array of int /* ERROR */
_Optional TAI oai; // as above /* ERROR */
_Optional int oai[2][3]; // as above /* ERROR */
TAOI aoi; // array of optional int /* ERROR */
TOI aoi[2][3]; // as above /* ERROR */
#endif

#ifdef TEST63
// valid: referenced type is qualified
_Optional int *frpoi(float);
_Optional int (*frpaoi(void))[10];

// invalid: qualified type is not a referenced type
_Optional int froi(float); /* ERROR */
_Optional int (*pfroi)(float); /*IGNORE */ // BUG - fix this!
#endif

#ifdef TEST64
#if 0 // missing support for _Optional for function types
// valid: referenced type is qualified
void fpoi( _Optional int *poi);
void fpaoi(_Optional int (*paoi)[2][3]);
void fpofri(_Optional typeof(int (float)) *pofri);

// valid after adjustment to pointer-to-array type
void faoi(_Optional int aoi[2][3]);
static_assert(_Generic(faoi,
              void (*)(_Optional int (*)[3]):1,
              default:0));

// valid after adjustment to pointer-to-function type
void fofri(_Optional typeof(int (float)) ofri);
static_assert(_Generic(fofri,
           void (*)(_Optional typeof(int (float)) *):1,
           default:0));

// invalid: qualified type is not a referenced type
void foi( _Optional int oi);
void fopi(int *_Optional opi);
void fopai(int (*_Optional opai)[2][3]);
#endif
#endif

#ifdef TEST65
int sum(_Optional int poi[static 4])
{
  int tot = 0;
  if (poi) {
    for (int i = 0; i < 4; ++i)
      tot += poi[i];
  }
  return tot;
}

int main(void)
{
  return sum(nullptr); // valid (returns 0)
}
#endif

#ifdef TEST66
typedef int TFRI(float);

// valid: does not declare an object
typedef _Optional int TOI;
typedef TOI X;
typedef int *_Optional TOPI;
typedef _Optional int *TPOI;
typedef int (*_Optional TOPFRI)(float);
typedef _Optional TFRI *TPOFRI;

// valid: referenced type is qualified
TOI *poi;       // treat poi as potentially null
TOPI *popi;     // treat popi but not *popi as potentially null
TPOI poi;       // treat poi as potentially null
TOPFRI *popfri; // treat popfri but not *popfri as maybe null
TPOFRI pofri;   // treat pofri as potentially null
 
// invalid: qualified type is not a referenced type
TOI oi; /* ERROR */
X oi; /* ERROR */
TOPI opi; /* ERROR */
TOPFRI opfri; /* ERROR */
#endif

#ifdef TEST67
#if 0 // incomplete support
typedef int TFRI(float);

// valid: does not declare a function
typedef _Optional TFRI TOFRI;
typedef TOFRI Y;

// valid: referenced type is qualified
TOFRI *pofri; // treat pofri as potentially null

// invalid: qualified type is not a referenced type
TOFRI ofri;
Y ofri;
#endif
#endif

#ifdef TEST68
// valid: does not declare an array
typedef _Optional int TAOI[2][3];

// valid: parameter type is adjusted to pointer type
void faoi(TAOI aoi);
static_assert(_Generic(faoi,
                       void (*)(
                         _Optional int (*)[3]
                       ): 1,
default: 0));

// invalid: array type is not adjusted to pointer type
TAOI aoi; /* ERROR */
#endif

#ifdef TEST69
#if 0 // incomplete support for function types
// valid: does not declare a function
typedef _Optional typeof(int (float)) TOFRI;

// valid: parameter type is adjusted to pointer type
void faoi(TOFRI ofri);
static_assert(_Generic(faoi,
                       void (*)(
                         _Optional typeof(int (float)) *
                       ): 1,
default: 0));

// invalid: function type is not adjusted to pointer type
TOFRI ofri;
#endif
#endif

#ifdef TEST70
char *npc = nullptr;            // valid but unsafe /* IGNORE */
char *lpc = "hello";            // valid but unsafe /* WARNING */

_Optional char *npoc = nullptr; // valid and safe
const char *lpcc = "hello";     // valid and safe
#endif

#ifdef TEST71
static char *pc;                       // valid but unsafe /* IGNORE */
char *apc[2] = {&(char){}};            // valid but unsafe /* IGNORE */

static _Optional char *poc;            // valid and safe
_Optional char *apoc[2] = {&(char){}}; // valid and safe /* IGNORE */ // TODO: bug?
#endif

#ifdef TEST72
int main(void)
{
  char *npc, *lpc;
  _Optional char *poc;
  const char *pcc;

  npc = nullptr; // valid but unsafe /* IGNORE */
  lpc = "hello"; // valid but unsafe /* WARNING */

  poc = nullptr; // valid and safe
  pcc = "world"; // valid and safe

  return 0;
}
#endif

