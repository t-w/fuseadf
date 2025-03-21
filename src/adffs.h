
#ifndef ADFFS_H
#define ADFFS_H

#include "adffs_fuse_api.h"
//#include "adflib.h"
#include "adfimage.h"
#include <stdio.h>

//#define DEBUG_ADFFS 1

//
// adffs state
//
typedef
struct adffs_state {
    char *       mountpoint;
    adfimage_t * adfimage;
    FILE *       logfile;
} adffs_state_t;

static inline struct adffs_state * adffs_get_state(void)
{
    return ( struct adffs_state * ) fuse_get_context()->private_data;
}


//
// adffs functions for FUSE
//
void * adffs_init ( struct fuse_conn_info * conn );
void adffs_destroy ( void * private_data );
int adffs_statfs ( const char *     path,
                   struct statvfs * stvfs );

int adffs_getattr ( const char *  path,
                    struct stat * statbuf );

int adffs_read ( const char *            path,
                 char *                  buffer,
                 size_t                  size,
                 off_t                   offset,
                 struct fuse_file_info * finfo );

int adffs_readdir ( const char *            path,
                    void *                  buffer,
                    fuse_fill_dir_t         filler,
                    off_t                   offset,
                    struct fuse_file_info * finfo );

extern struct fuse_operations adffs_oper;

#endif
