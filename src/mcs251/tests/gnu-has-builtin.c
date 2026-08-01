#ifndef __has_builtin
#error "__has_builtin must be available"
#endif

#if !__has_builtin (__builtin_offsetof)
#error "__builtin_offsetof must be reported"
#endif

#if !__has_builtin (__builtin_unreachable)
#error "__builtin_unreachable must be reported"
#endif

#if __has_builtin (__builtin_not_implemented)
#error "unknown builtins must not be reported"
#endif

#ifdef __SDCC_GNU_EXTENSIONS
#if !__has_builtin (__builtin_constant_p)
#error "__builtin_constant_p must be reported in GNU modes"
#endif
#if !__has_builtin (__builtin_expect)
#error "__builtin_expect must be reported in GNU modes"
#endif
#if !__has_builtin (__builtin_types_compatible_p)
#error "__builtin_types_compatible_p must be reported in GNU modes"
#endif
#else
#if __has_builtin (__builtin_constant_p)
#error "__builtin_constant_p must not be reported in strict modes"
#endif
#if __has_builtin (__builtin_expect)
#error "__builtin_expect must not be reported in strict modes"
#endif
#if __has_builtin (__builtin_types_compatible_p)
#error "__builtin_types_compatible_p must not be reported in strict modes"
#endif
#endif

int
gnu_has_builtin (void)
{
  return 0;
}
