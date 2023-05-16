

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

AM_CFLAGS = -Wall -Wextra -pedantic -Wconversion -Wsign-conversion \
  -Werror-implicit-function-declaration \
  -Werror=incompatible-pointer-types \
  @FUSE_CFLAGS@ @ADF_CFLAGS@

LDADD = @FUSE_LIBS@ @ADF_LIBS@
