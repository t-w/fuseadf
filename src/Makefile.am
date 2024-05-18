
AM_CFLAGS = -Wall -Wextra \
    -pedantic \
    -pedantic-errors \
    -Wconversion \
    -Wsign-conversion \
    -Werror-implicit-function-declaration \
    -Werror=incompatible-pointer-types \
    -Werror=format-security \
    @FUSE_CFLAGS@ \
    @ADF_CFLAGS@

if USE_AS_ROOT
AM_CPPFLAGS = -DALLOW_USE_AS_ROOT
endif

bin_PROGRAMS = fuseadf

fuseadf_SOURCES = fuseadf.c \
  config.h \
  adfimage.c \
  adfimage.h \
  adffs.c \
  adffs.h \
  adffs_util.c \
  adffs_util.h \
  adffs_log.c \
  adffs_log.h \
  log.c \
  log.h

LDADD = @FUSE_LIBS@ @ADF_LIBS@
