
AC_PREREQ([2.54])

AC_INIT([fuseadf], [0.2], [tomas.wolak@gmail.com])

# https://www.gnu.org/software/automake/manual/html_node/List-of-Automake-options.html
AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects])

AC_CONFIG_SRCDIR([src/fuseadf.c])
AC_CONFIG_HEADERS([src/config.h])

AC_PROG_CC

AC_CHECK_HEADERS([errno.h inttypes.h limits.h stdarg.h stdio.h stdint.h \
    stdlib.h string.h sys/stat.h sys/statvfs.h sys/types.h unistd.h])

PKG_CHECK_MODULES(FUSE, fuse >= 2.9)
PKG_CHECK_MODULES(ADF, adflib >= 0.7.13)
PKG_CHECK_MODULES([CHECK], [check >= 0.9.6])

AC_TYPE_UID_T
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_CHECK_MEMBERS([struct stat.st_blksize])
AC_STRUCT_ST_BLOCKS
AC_CHECK_MEMBERS([struct stat.st_rdev])
AC_TYPE_UINT64_T

AC_FUNC_MALLOC
#AC_CHECK_FUNCS([realpath strerror])

AC_CONFIG_FILES([
    Makefile
    src/Makefile
    tests/Makefile])

AC_OUTPUT
