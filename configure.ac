
AC_PREREQ([2.69])

AC_INIT([sleep-until],[1.0],[https://github.com/haukex/sleep_until/issues],[sleep_until],[https://pypi.org/project/sleep-until/])

AC_CONFIG_HEADERS([config.h])

AC_CHECK_FUNCS(clock_nanosleep, [], [
    AC_CHECK_LIB(rt, clock_nanosleep, [
        AC_DEFINE(HAVE_CLOCK_NANOSLEEP, 1)
    ])
])

AC_CHECK_FUNCS(nanosleep, [], [
    AC_CHECK_LIB(rt, nanosleep, [
        AC_DEFINE(HAVE_NANOSLEEP, 1)
    ])
])

AC_OUTPUT

