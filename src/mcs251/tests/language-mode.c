#ifndef EXPECTED_STDC_VERSION
#error EXPECTED_STDC_VERSION is not defined
#endif

#ifndef EXPECT_STRICT_ANSI
#error EXPECT_STRICT_ANSI is not defined
#endif

#ifndef __STDC_VERSION__
#error __STDC_VERSION__ is not defined
#elif __STDC_VERSION__ != EXPECTED_STDC_VERSION
#error __STDC_VERSION__ does not match the selected language mode
#endif

#if EXPECT_GNU_EXTENSIONS
#ifndef __SDCC_GNU_EXTENSIONS
#error __SDCC_GNU_EXTENSIONS is not defined in a GNU mode
#endif
#else
#ifdef __SDCC_GNU_EXTENSIONS
#error __SDCC_GNU_EXTENSIONS is defined in a strict ISO mode
#endif
#endif

#if EXPECT_STRICT_ANSI
#ifndef __STRICT_ANSI__
#error __STRICT_ANSI__ is not defined in a strict ISO mode
#endif
#else
#ifdef __STRICT_ANSI__
#error __STRICT_ANSI__ is defined in an extension mode
#endif
#endif

int
language_mode_probe (void)
{
  return 0;
}
