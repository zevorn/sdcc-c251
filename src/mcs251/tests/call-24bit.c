volatile unsigned char call_result;

unsigned char
mcs251_callee (unsigned char value)
{
    return value + 1;
}

typedef unsigned char (*mcs251_unary_function) (unsigned char);

const mcs251_unary_function mcs251_function_table[] = {
    mcs251_callee
};

unsigned long
mcs251_two_argument_callee (unsigned long value, unsigned char increment)
{
    return value + increment;
}

unsigned long
mcs251_forward_four_bytes (unsigned long value)
{
    return mcs251_two_argument_callee (value, 1);
}

unsigned char
mcs251_caller (unsigned char value)
{
    unsigned char result = mcs251_callee (value);

    call_result = result;
    return result;
}

unsigned char
mcs251_indirect_caller (unsigned char (*function) (unsigned char),
                      unsigned char value)
{
    return function (value);
}

_Noreturn void mcs251_stop (void);
_Noreturn void mcs251_stop_with_stack_argument (unsigned char first,
                                               unsigned int second) __reentrant;

void
mcs251_noreturn (void)
{
    mcs251_stop ();
}

_Noreturn void
mcs251_noreturn_with_stack_argument (void)
{
    mcs251_stop_with_stack_argument (1, 0x1234);
}
