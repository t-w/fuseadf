
TESTS = \
    test_adfimage \
    test_time_to_time_t

check_PROGRAMS = \
    test_adfimage \
    test_time_to_time_t

AM_CFLAGS = -Wall -Wextra -pedantic -pedantic-errors \
    -Wconversion -Wsign-conversion \
    -Werror-implicit-function-declaration \
    -Werror=incompatible-pointer-types \
    -I$(srcdir) \
    -I$(top_srcdir)/src \
    @ADF_CFLAGS@ \
    @CHECK_CFLAGS@

test_adfimage_SOURCES = test_adfimage.c \
    ../src/adfimage.c \
    ../src/adfimage.h
test_adfimage_LDADD = \
    @ADF_LIBS@ \
    @CHECK_LIBS@

test_time_to_time_t_SOURCES = test_time_to_time_t.c \
    ../src/adffs_util.c \
    ../src/adffs_util.h
test_time_to_time_t_LDADD = \
    @CHECK_LIBS@
