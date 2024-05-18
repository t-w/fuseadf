
AM_CFLAGS = -Wall -Wextra \
    -pedantic \
    -pedantic-errors \
    -Wconversion \
    -Wsign-conversion \
    -Werror-implicit-function-declaration \
    -Werror=incompatible-pointer-types \
    -Werror=format-security\
    -I$(srcdir) \
    -I$(top_srcdir)/src \
    @CHECK_CFLAGS@

#AM_CPPFLAGS =

dist_check_SCRIPTS = \
    prepare_test_data.sh \
    remove_test_data.sh

check_SCRIPTS = $(dist_check_SCRIPTS)

TESTS = \
    prepare_test_data.sh \
    test_adfimage \
    test_time_to_time_t \
    remove_test_data.sh

check_PROGRAMS = \
    test_adfimage \
    test_time_to_time_t


test_adfimage_SOURCES = test_adfimage.c \
    ../src/adfimage.c \
    ../src/adfimage.h \
    ../src/adffs_log.c \
    ../src/adffs_log.h \
    ../src/log.c \
    ../src/log.h

test_adfimage_CFLAGS = \
    $(AM_CFLAGS) \
    @ADF_CFLAGS@ \
    @FUSE_CFLAGS@

test_adfimage_LDADD = \
    @ADF_LIBS@ \
    @CHECK_LIBS@


test_time_to_time_t_SOURCES = test_time_to_time_t.c \
    ../src/adffs_util.c \
    ../src/adffs_util.h
test_time_to_time_t_LDADD = \
    @CHECK_LIBS@
