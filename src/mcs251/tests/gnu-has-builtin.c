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
#if !__has_builtin (__builtin_clz) || !__has_builtin (__builtin_clzl) || \
    !__has_builtin (__builtin_clzll)
#error "leading-zero builtins must be reported in GNU modes"
#endif
#if !__has_builtin (__builtin_ctz) || !__has_builtin (__builtin_ctzl) || \
    !__has_builtin (__builtin_ctzll)
#error "trailing-zero builtins must be reported in GNU modes"
#endif
#if !__has_builtin (__builtin_popcount) || \
    !__has_builtin (__builtin_popcountl) || \
    !__has_builtin (__builtin_popcountll)
#error "population-count builtins must be reported in GNU modes"
#endif
#if !__has_builtin (__builtin_ffs) || !__has_builtin (__builtin_ffsl) || \
    !__has_builtin (__builtin_ffsll)
#error "find-first-set builtins must be reported in GNU modes"
#endif
#if !__has_builtin (__builtin_bswap16) || \
    !__has_builtin (__builtin_bswap32) || \
    !__has_builtin (__builtin_bswap64)
#error "byte-swap builtins must be reported in GNU modes"
#endif
#if !__has_builtin (__builtin_add_overflow) || \
    !__has_builtin (__builtin_sub_overflow) || \
    !__has_builtin (__builtin_mul_overflow)
#error "overflow builtins must be reported in GNU modes"
#endif
#if !__has_builtin (__builtin_sadd_overflow) || \
    !__has_builtin (__builtin_saddl_overflow) || \
    !__has_builtin (__builtin_saddll_overflow) || \
    !__has_builtin (__builtin_uadd_overflow) || \
    !__has_builtin (__builtin_uaddl_overflow) || \
    !__has_builtin (__builtin_uaddll_overflow)
#error "typed add overflow builtins must be reported in GNU modes"
#endif
#if !__has_builtin (__builtin_ssub_overflow) || \
    !__has_builtin (__builtin_ssubl_overflow) || \
    !__has_builtin (__builtin_ssubll_overflow) || \
    !__has_builtin (__builtin_usub_overflow) || \
    !__has_builtin (__builtin_usubl_overflow) || \
    !__has_builtin (__builtin_usubll_overflow)
#error "typed subtract overflow builtins must be reported in GNU modes"
#endif
#if !__has_builtin (__builtin_smul_overflow) || \
    !__has_builtin (__builtin_smull_overflow) || \
    !__has_builtin (__builtin_smulll_overflow) || \
    !__has_builtin (__builtin_umul_overflow) || \
    !__has_builtin (__builtin_umull_overflow) || \
    !__has_builtin (__builtin_umulll_overflow)
#error "typed multiply overflow builtins must be reported in GNU modes"
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
#if __has_builtin (__builtin_clz) || __has_builtin (__builtin_ctz) || \
    __has_builtin (__builtin_popcount) || __has_builtin (__builtin_ffs)
#error "bit-count builtins must not be reported in strict modes"
#endif
#if __has_builtin (__builtin_bswap16) || \
    __has_builtin (__builtin_bswap32) || \
    __has_builtin (__builtin_bswap64)
#error "byte-swap builtins must not be reported in strict modes"
#endif
#if __has_builtin (__builtin_add_overflow) || \
    __has_builtin (__builtin_sub_overflow) || \
    __has_builtin (__builtin_mul_overflow)
#error "overflow builtins must not be reported in strict modes"
#endif
#if __has_builtin (__builtin_sadd_overflow) || \
    __has_builtin (__builtin_saddl_overflow) || \
    __has_builtin (__builtin_saddll_overflow) || \
    __has_builtin (__builtin_uadd_overflow) || \
    __has_builtin (__builtin_uaddl_overflow) || \
    __has_builtin (__builtin_uaddll_overflow)
#error "typed add overflow builtins must not be reported in strict modes"
#endif
#if __has_builtin (__builtin_ssub_overflow) || \
    __has_builtin (__builtin_ssubl_overflow) || \
    __has_builtin (__builtin_ssubll_overflow) || \
    __has_builtin (__builtin_usub_overflow) || \
    __has_builtin (__builtin_usubl_overflow) || \
    __has_builtin (__builtin_usubll_overflow)
#error "typed subtract overflow builtins must not be reported in strict modes"
#endif
#if __has_builtin (__builtin_smul_overflow) || \
    __has_builtin (__builtin_smull_overflow) || \
    __has_builtin (__builtin_smulll_overflow) || \
    __has_builtin (__builtin_umul_overflow) || \
    __has_builtin (__builtin_umull_overflow) || \
    __has_builtin (__builtin_umulll_overflow)
#error "typed multiply overflow builtins must not be reported in strict modes"
#endif
#endif

int
gnu_has_builtin (void)
{
  return 0;
}
