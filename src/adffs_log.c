
#include "adffs_log.h"

#include "adffs.h"
#include "log.h"

#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>


// struct stat:
//   /usr/include/bits/struct_stat.h
void log_stat ( const struct stat * const ststat )
{
    log_info (
        "\nstat {\n"
        "    .st_dev     = %lld\n"
        "    .st_ino     = %lld\n"
        "    .st_mode    = 0%o\n"
        "    .st_nlink   = %d\n"
        "    .st_uid     = %d\n"
        "    .st_gid     = %lld\n"
        "    .st_rdev    = %lld\n"
        "    .st_size    = %lld\n"
        "    .st_blksize = %ld\n"
        "    .st_blocks  = %lld\n"
        "    .st_atime   = 0x%08lx\n"
        "    .st_mtime   = 0x%08lx\n"
        "    .st_ctime   = 0x%08lx }\n"
        , ststat->st_dev
        , ststat->st_ino
        , ststat->st_mode
        , ststat->st_nlink
        , ststat->st_uid
        , ststat->st_gid
        , ststat->st_rdev
        , ststat->st_size
        , ststat->st_blksize
        , ststat->st_blocks
        , ststat->st_atime
        , ststat->st_mtime
        , ststat->st_ctime );
}

// struct statvfs:
//   /usr/include/bits/statvfs.h
void log_statvfs ( const struct statvfs * const stvfs )
{
    log_info (
        "\nstatvfs {\n"
        "    .f_bsize   = %ld\n"
        "    .f_frsize  = %ld\n"
        "    .f_blocks  = %lld\n"
        "    .f_bfree   = %lld\n"
        "    .f_bavail  = %lld\n"
        "    .f_files   = %lld\n"
        "    .f_ffree   = %lld\n"
        "    .f_favail  = %lld\n"
        "    .f_fsid    = %ld\n"
        "    .f_flag    = 0x%08lx\n"
        "    .f_namemax = %ld } \n",
        stvfs->f_bsize,
        stvfs->f_frsize,
        stvfs->f_blocks,
        stvfs->f_bfree,
        stvfs->f_bavail,
        stvfs->f_files,
        stvfs->f_ffree,
        stvfs->f_favail,
        stvfs->f_fsid,
        stvfs->f_flag,
        stvfs->f_namemax );
}


// struct fuse_context:
//   https://github.com/libfuse/libfuse/blob/master/include/fuse.h#L814
void log_fuse_context ( const struct fuse_context * const context )
{    
    log_info (
        "\nfuse_context {\n"
        "    .fuse  = 0x%" PRIxPTR "\n"
        "    .uid   = %d\n"
        "    .gid   = %d\n"
        "    .pid   = %d\n"
        "    .umask = %05o\n"
        , context->fuse
        , context->uid
        , context->gid
        , context->pid
        , context->umask );

    // private data
    const struct adffs_state * const private_data =
        ( struct adffs_state * ) context->private_data;
    log_info (
        "    .private_data = 0x%" PRIxPTR "\n"
        "    .logfile      = 0x%" PRIxPTR "\n"
        "    .mountpoint   = %s } \n"
        , private_data
        , private_data->logfile
        , private_data->mountpoint );
}

// struct fuse_conn_info
//   https://github.com/libfuse/libfuse/blob/master/include/fuse_common.h#L425
void log_fuse_conn_info ( const struct fuse_conn_info * const conn )
{
    log_info (
        "\nfuse_conn_info {\n"
        "    .proto_major          = %d\n"
        "    .proto_minor          = %d\n"
        "    .async_read           = %d\n"
        "    .max_write            = %d\n"
        "    .max_readahead        = %d\n"
        "    .capable              = 0x%08x\n"
        "    .want                 = 0x%08x\n"
        "    .max_background       = %d\n"
        "    .congestion_threshold = %d\n"
        , conn->proto_major
        , conn->proto_minor
        , conn->async_read
        , conn->max_write
        , conn->max_readahead
        , conn->capable
        , conn->want
        , conn->max_background
        , conn->congestion_threshold );
}


// struct fuse_file_info:
//   https://github.com/libfuse/libfuse/blob/master/include/fuse_common.h#L44
// or (better) /usr/include/fuse/fuse_common.h as it may vary...
void log_fuse_file_info ( const struct fuse_file_info * const finfo )
{
    log_info (
        "\nfuse_file_info { \n"
        "    .flags         = 0x%08x\n"
        "    .writepage     = %d\n"
        "    .direct_io     = %d\n"
        "    .keep_cache    = %d\n"
        "    .flush         = %d\n"
        "    .nonseekable   = %d\n"
        "    .flock_release = %d\n"
        //"  .cache_redir   = %d\n"
        //"  .noflush       = %d\n"
        // .padding 24
        // .padding 32
        "    .fh            = 0x%016llx\n"
        "    .lock_owner    = 0x%016llx }\n"
        //"  .poll_events   = %d\n"
        , finfo->flags
        , finfo->writepage
        , finfo->direct_io
        , finfo->keep_cache
        , finfo->flush
        , finfo->nonseekable
        , finfo->flock_release
        //, finfo->cache_redir
        //, finfo->noflush
        , finfo->fh
        , finfo->lock_owner
        //finfo->poll_events
        );
}
