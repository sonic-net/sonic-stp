AC_INIT([sonic-stp],[1.0.0])
AC_CONFIG_SRCDIR([])
AC_CONFIG_AUX_DIR(config)
AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE([foreign])
AC_LANG_C
AC_LANG([C++])
AC_PROG_CC
AC_PROG_CXX
AC_PROG_LIBTOOL
AC_HEADER_STDC

#AC_CHECK_LIB([hiredis], [redisConnect],,
#    AC_MSG_ERROR([libhiredis is not installed.]))

#AC_CHECK_LIB([nl-genl-3], [genl_connect])

AC_ARG_ENABLE(debug,
[  --enable-debug      Compile with debugging flags],
[case "${enableval}" in
        yes) debug=true ;;
        no)  debug=false ;;
        *) AC_MSG_ERROR(bad value ${enableval} for --enable-debug) ;;
esac],[debug=false])
AM_CONDITIONAL(DEBUG, test x$debug = xtrue)

dnl Build the unit tests wherever googletest is present, so `make check'
dnl runs them. --disable-gtest opts out; --enable-gtest makes a missing
dnl header a hard error.
AC_ARG_ENABLE(gtest,
[  --enable-gtest       Build the googletest unit tests (default: auto)],
[case "${enableval}" in
        yes) gtest=true ;;
        no)  gtest=false ;;
        *) AC_MSG_ERROR(bad value ${enableval} for --enable-gtest) ;;
esac],[gtest=auto])

if test x$gtest != xfalse; then
    AC_CHECK_HEADER([gtest/gtest.h], [gtest_header=yes], [gtest_header=no])
    if test x$gtest_header = xyes; then
        gtest=true
    elif test x$gtest = xtrue; then
        AC_MSG_ERROR([--enable-gtest was given but gtest/gtest.h was not found])
    else
        AC_MSG_NOTICE([gtest/gtest.h not found - unit tests will not be built])
        gtest=false
    fi
fi
AM_CONDITIONAL(GTEST, test x$gtest = xtrue)

CFLAGS_COMMON="-std=c++11 -Wall -fPIC -Wno-write-strings -I/usr/include/libnl3 -I/usr/include/swss -I/usr/include"

CFLAGS_COMMON+=" -Werror"

AC_SUBST(CFLAGS_COMMON)

AC_CONFIG_FILES([
    include/Makefile
    lib/Makefile
    stpctl/Makefile
    Makefile
])

AC_OUTPUT

