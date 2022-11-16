
#ifndef __ADFFS_LOG_H__
#define __ADFFS_LOG_H__

#include "adffs_fuse_api.h"

void log_stat    ( const struct stat * const    ststat );
void log_statvfs ( const struct statvfs * const stvfs );

void log_fuse_context   ( const struct fuse_context * const   context );
void log_fuse_conn_info ( const struct fuse_conn_info * const conninfo );
void log_fuse_file_info ( const struct fuse_file_info * const finfo );

#endif
