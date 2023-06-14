
#ifndef __ADFFS_LOG_H__
#define __ADFFS_LOG_H__

#include "adffs_fuse_api.h"
#include <stdio.h>

FILE * adffs_log_open ( const char * const log_file_path );
void adffs_log_close ( void );

void adffs_log_info ( const char * const format, ... );

void adffs_log_stat    ( const struct stat * const    ststat );
void adffs_log_statvfs ( const struct statvfs * const stvfs );

void adffs_log_fuse_context   ( const struct fuse_context * const   context );
void adffs_log_fuse_conn_info ( const struct fuse_conn_info * const conninfo );
void adffs_log_fuse_file_info ( const struct fuse_file_info * const finfo );

#endif
