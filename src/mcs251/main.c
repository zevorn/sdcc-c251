/*
 * The MCS251 port shares the mature MCS-51 code generator while supplying a
 * distinct target description, assembler command, and optimization rules.
 * Native MCS251 lowering is selected in the shared backend with MCS251_PORT.
 */
#define MCS251_PORT 1
#include "../mcs51/main.c"
