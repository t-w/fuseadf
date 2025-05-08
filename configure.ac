
AC_PREREQ([2.54])

AC_INIT([fuseadf], [0.7],
        [tomas.wolak@gmail.com or (better) https://gitlab.com/t-m/fuseadf/-/issues],
        [], [https://gitlab.com/t-m/fuseadf])

AC_ARG_ENABLE([use_as_root],
              [  --enable-use-as-root    Permit using as root (default: no)],
              [case "${enableval}" in
                yes) use_as_root=true ;;
                no)  use_as_root=false ;;
                *) AC_MSG_ERROR([bad value ${enableval} for --enable-use-as-root]) ;;
               esac],
              [use_as_root=false])

AM_CONDITIONAL([USE_AS_ROOT], [test x$use_as_root = xtrue])
echo "Permit use as root: ${use_as_root}"

# https://www.gnu.org/software/automake/manual/html_node/List-of-Automake-options.html
AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects])

AC_CONFIG_SRCDIR([src/fuseadf.c])
AC_CONFIG_HEADERS([src/config.h])

AC_PROG_CC

AC_CHECK_HEADERS([errno.h inttypes.h limits.h stdarg.h stdio.h stdint.h \
    stdlib.h string.h sys/stat.h sys/statvfs.h sys/types.h unistd.h])

PKG_CHECK_MODULES(FUSE, fuse >= 2.9)
PKG_CHECK_MODULES(ADF, adflib >= 0.10.0)
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
