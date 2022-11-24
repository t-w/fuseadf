
#include "adffs.h"

#include "config.h"
//#include "cdimage.h"

#include <errno.h>
#include <inttypes.h>
#include <libgen.h>
#include <stdlib.h>
#include <string.h>

//#define __USE_GNU
//#include <sys/statfs.h>
//#undef __USE_GNU

#include <sys/types.h>
#include <unistd.h>

#ifdef DEBUG_ADFFS
#include "log.h"
#include "adffs_log.h"
#endif


/*******************************************************
 * Filesystem functions (init / destroy / statfs / ...
 *******************************************************/

void * adffs_init ( struct fuse_conn_info * conninfo )
{
    struct fuse_context * const context = fuse_get_context();
    const adffs_state_t * const fs_state =
        ( adffs_state_t * ) context->private_data;

#ifdef DEBUG_ADFFS
    log_info ( fs_state->logfile, "\nadffs_init ( conninfo = 0x%" PRIxPTR " )\n",
               conninfo );
    log_fuse_conn_info ( conninfo );
#endif

#ifdef DEBUG_ADFFS
    log_fuse_context ( context );
#endif

    return context->private_data;
}


void adffs_destroy ( void * private_data )
{
    adffs_state_t * const fs_state = ( adffs_state_t * ) private_data;
    
#ifdef DEBUG_ADFFS
    log_info ( fs_state->logfile,
               "\nadffs_destroy ( userdata = 0x%" PRIxPTR " )\n", private_data );
#endif

    if ( fs_state->adfimage )
        adfimage_close ( &fs_state->adfimage );

    free ( fs_state->mountpoint );
    fs_state->mountpoint = NULL;
}


int adffs_statfs ( const char *     path,
                   struct statvfs * stvfs )
{
    const adffs_state_t * const fs_state =
        ( adffs_state_t * ) fuse_get_context()->private_data;

#ifdef DEBUG_ADFFS 
    log_info ( fs_state->logfile,
               "\nadffs_statfs (\n"
               "    path  = \"%s\",\n"
               "    statv = 0x%" PRIxPTR " )\n",
               path, stvfs );
#endif

    stvfs->f_flag = ST_RDONLY | ST_NOSUID;
//        | ST_NODEV | ST_NOEXEC | ST_IMMUTABLE | ST_NOATIME | ST_NODIRATIME;
    // ^^^^ for some reason these are not available here???
    // <sys/statvfs.h> 
    stvfs->f_bfree =
    stvfs->f_bavail =
    stvfs->f_ffree = 0;

#ifdef DEBUG_ADFFS
    log_statvfs ( stvfs );
#endif

    return 0;
}



/*******************************************************
 * File and directory operations ( read / stat / ... )
 *******************************************************/

int adffs_getattr ( const char *  path,
                    struct stat * statbuf )
{
    adffs_state_t * const fs_state =
        ( adffs_state_t * ) fuse_get_context()->private_data;

#ifdef DEBUG_ADFFS
    log_info ( fs_state->logfile,
               "\nadffs_getattr (\n"
               "    path    = \"%s\",\n"
               "    statbuf = 0x%" PRIxPTR " )\n",
          path, statbuf );
#endif

    memset ( statbuf, 0, sizeof ( *statbuf ) );

    while ( *path == '/' ) // skip all leading '/' from the path
        path++;            // (normally, fuse always starts with a single '/')

    if ( *path == '\0' ) {
        // main directory
        statbuf->st_mode = S_IFDIR |
            S_IRUSR | S_IXUSR |
            S_IRGRP | S_IXGRP |
            S_IROTH | S_IXOTH;
            
        //statbuf->st_size = cdimage->ntracks;

        // links count - always 1 (what should it be?)
        statbuf->st_nlink = 1;

    } else {         // path is a non-empty string
                     // so anything besided main directory

        adfimage_t * const adfimage = fs_state->adfimage;
        struct Volume * const vol = adfimage->vol;

        // first, find and enter the directory where is dir. entry to check
        char * dirpath_buf = strdup ( path );
        char * dir_path = dirname ( dirpath_buf );
        if ( ! adfimage_chdir ( adfimage, dir_path ) ) {
            log_info ( fs_state->logfile, "adffs_getattr(): Cannot chdir to the directory %s.\n",
                       dir_path );
        }
        free ( dirpath_buf );

        char * direntry_buf = strdup ( path );
        char * direntry_name = basename ( direntry_buf );
        if ( *direntry_name == '\0' ) {
            // empty name means that given path is a directory
            // to which we entered above - so we need to check
            // properties of the current directory
            direntry_name = ".";
        }

        adfimage_dentry_t dentry = adfimage_getdentry ( adfimage, direntry_name );

        if ( dentry.type == ADFVOLUME_DENTRY_FILE ) {
            statbuf->st_mode = S_IFREG |
                S_IRUSR |
                S_IRGRP |
                S_IROTH;
            statbuf->st_nlink = 1;

            struct File * afile = adfOpenFile ( vol, direntry_name, "r" );
            free (direntry_buf);
            if ( afile ) {
                statbuf->st_size = afile->fileHdr->byteSize;
            } else
                log_info ( fs_state->logfile,
                           "adffs_getattr(): Error opening file: %s\n", path );

        } else if ( dentry.type == ADFVOLUME_DENTRY_DIRECTORY ) {
            free ( direntry_buf );
            statbuf->st_mode = S_IFDIR |
                S_IRUSR | S_IXUSR |
                S_IRGRP | S_IXGRP |
                S_IROTH | S_IXOTH;
            statbuf->st_nlink = 1;

        } else {
            // file/dirname not found
            free ( direntry_buf );
            adfToRootDir ( vol );
            return -ENOENT;
        }            
        adfToRootDir ( vol );
    }

    statbuf->st_uid = geteuid();
    statbuf->st_gid = getegid();

    //statbuf->st_atime = cdimage->fstat.st_atime;
    //statbuf->st_mtime = cdimage->fstat.st_mtime;
    //statbuf->st_ctime = cdimage->fstat.st_ctime;

#ifdef DEBUG_ADFFS
    log_stat ( statbuf );
#endif

    return 0;
}


int adffs_read ( const char *            path,
                 char *                  buffer,
                 size_t                  size,
                 off_t                   offset,
                 struct fuse_file_info * finfo )
{
    const adffs_state_t * const fs_state =
        ( adffs_state_t * ) fuse_get_context()->private_data;
    
#ifdef DEBUG_ADFFS
    log_info ( fs_state->logfile,
               "\nadffs_read (\n"
               "    path   = \"%s\",\n"
               "    buf    = 0x%" PRIxPTR ",\n"
               "    size   = %d,\n"
               "    offset = %lld,\n"
               "    finfo  = 0x%" PRIxPTR " )\n",
               path, buffer, size, offset, finfo );
#endif

    int bytes_read = adfimage_read ( fs_state->adfimage, path,
                                     buffer, size, offset );

#ifdef DEBUG_ADFFS
    //log_info ( fs_state->logfile,
    //           "adffs_read () => %d (%s)\n", bytes_read,
    //           size == (size_t) bytes_read ? "OK" : "READ ERROR" );
#endif

    return bytes_read;
}


int adffs_readdir ( const char *            path,
                    void *                  buffer,
                    fuse_fill_dir_t         filler,
                    off_t                   offset,
                    struct fuse_file_info * finfo )
{
    const adffs_state_t * const fs_state =
        ( adffs_state_t * ) fuse_get_context()->private_data;

#ifdef DEBUG_ADFFS
    log_info ( fs_state->logfile,
               "\nadffs_readdir (\n"
               "    path   = \"%s\",\n"
               "    buf    = 0x%" PRIxPTR ",\n"
               "    filler = 0x%"  PRIxPTR ",\n"
               "    offset = %lld,\n"
               "    finfo  = 0x%" PRIxPTR " )\n",
               path, buffer, filler, offset, finfo );
#endif
    adfimage_t * const adfimage = fs_state->adfimage;
    struct Volume * const vol = adfimage->vol;
    if ( ! adfimage_chdir ( adfimage, path ) ) {
        log_info ( fs_state->logfile, "adffs_read(): Cannot chdir to the directory %s.\n",
                   path );
        adfToRootDir ( vol );
        return -ENOENT;
    }
    
    filler ( buffer, ".", NULL, 0 );
    filler ( buffer, "..", NULL, 0 );

    struct List * const dentries = adfGetDirEnt ( vol, vol->curDirPtr );
    if ( ! dentries ) {
        fprintf ( stderr, "adfimage_getdentry(): Error getting dir entries,"
                  "filename %s\n", path );
        return -ENOENT; //?
    }

    for ( struct List * lentry = dentries ;
          lentry ;
          lentry = lentry->next )
    {
        struct Entry * const dentry =
            ( struct Entry * ) lentry->content;
        //printf (" type: %d, size: %d, name: %s, comment: %s\n",
        //        dentry->type,
        //        dentry->size,
        //        dentry->name,
        //        dentry->comment );
        //printEntry(struct Entry* entry);
        //printEntry ( dentry );    
        if ( filler ( buffer, dentry->name, NULL, 0 ) ) {
            log_info ( fs_state->logfile, "adffs_readdir: filler: buffer full\n" );
            adfToRootDir ( vol );
            return -EAGAIN; // check what error return in such case(!)
                            // probably something from /usr/include/asm-generic/errno-base.h (?)
        }
    }

#ifdef DEBUG_ADFFS
    log_fuse_file_info ( finfo );
#endif

    adfToRootDir ( vol );
    return 0;
}

// struct fuse_operations: /usr/include/fuse/fuse.h
struct fuse_operations adffs_oper = {
    .getattr    = adffs_getattr,
    .readlink   = NULL,
    .getdir     = NULL,       // deprecated
    .mknod      = NULL,
    .mkdir      = NULL,
    .unlink     = NULL,
    .rmdir      = NULL,
    .symlink    = NULL,
    .rename     = NULL,
    .link       = NULL,
    .chmod      = NULL,
    .chown      = NULL,
    .truncate   = NULL,
    .utime      = NULL,
    .open       = NULL,
    .read       = adffs_read,
    .write      = NULL,
    .statfs     = adffs_statfs,
    .flush      = NULL,
    .release    = NULL,
    .fsync      = NULL,
    .opendir    = NULL,
    .readdir    = adffs_readdir,
    .releasedir = NULL,
    .fsyncdir   = NULL,
    .init       = adffs_init,
    .destroy    = adffs_destroy,
    .access     = NULL,
    .create     = NULL,
    .ftruncate  = NULL,
    .fgetattr   = NULL,
    .lock       = NULL,
    .utimens    = NULL,
    .bmap       = NULL,
    .ioctl      = NULL,
    .poll       = NULL,
    .write_buf  = NULL,
    .read_buf   = NULL,
    .flock      = NULL,
    .fallocate  = NULL
};
