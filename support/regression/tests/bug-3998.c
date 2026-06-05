/*
   bug-3997.c
   Leftover operand use info from not-quite-correctly-eliminated dead code triggered an assertion failure int eh compiler.
 */

#include <testfwk.h>

unsigned char loadedCharacters[10];

void my_func_1(unsigned char a);

inline void my_func2(unsigned int a){
    if (a < 256) {
        my_func_1((unsigned char)a);
    } else {
        my_func_1((unsigned char)a);
    }
}

void my_func3(void){    
    my_func2((unsigned int)loadedCharacters[1]);
}

void
testBug(void){
    my_func3();
}


extern void my_func2(unsigned int a);

#pragma disable_warning 85

void my_func_1(unsigned char a)
{
}

