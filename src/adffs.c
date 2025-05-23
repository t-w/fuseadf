
#include "adffs.h"

#include "config.h"
#include "adffs_util.h"

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

#include "log.h"
#include "adffs_log.h"


/*******************************************************
 * Filesystem functions (init / destroy / statfs / ...
 *******************************************************/

void * adffs_init ( struct fuse_conn_info * conninfo )
{
    struct fuse_context * const context = fuse_get_context();

#ifdef DEBUG_ADFFS
    const adffs_state_t * const fs_state =
        ( adffs_state_t * ) context->private_data;

    adffs_log_info ( "\nadffs_init ( conninfo = 0x%" PRIxPTR " )\n",
                     conninfo );
    adffs_log_fuse_conn_info( conninfo );
    adffs_log_fuse_context( context );
#else
    (void) conninfo;
#endif

    adffs_util_init();

    return context->private_data;
}


void adffs_destroy ( void * private_data )
{
    adffs_state_t * const fs_state = ( adffs_state_t * ) private_data;
    
#ifdef DEBUG_ADFFS
    adffs_log_info ( "\nadffs_destroy ( userdata = 0x%" PRIxPTR " )\n",
                     private_data );
#endif

    if ( fs_state->adfimage )
        adfimage_close ( &fs_state->adfimage );

    free ( fs_state->mountpoint );
    fs_state->mountpoint = NULL;

    adffs_log_close();
}


int adffs_statfs ( const char *     path,
                   struct statvfs * stvfs )
{
    (void) path;

    const adffs_state_t * const fs_state =
        ( adffs_state_t * ) fuse_get_context()->private_data;

#ifdef DEBUG_ADFFS
    adffs_log_info ( "\nadffs_statfs (\n"
                     "    path  = \"%s\",\n"
                     "    statv = 0x%" PRIxPTR " )\n",
                     path, stvfs );
#endif

    stvfs->f_flag = //ST_RDONLY |
        ST_NOSUID;
//        | ST_NODEV | ST_NOEXEC | ST_IMMUTABLE | ST_NOATIME | ST_NODIRATIME;
    // ^^^^ for some reason these are not available here???
    // <sys/statvfs.h>

    adfimage_t * const adfimage = fs_state->adfimage;
    struct AdfVolume * const vol = adfimage->vol;
    uint32_t blocks_free = adfCountFreeBlocks ( vol );

    if ( vol->readOnly )
        stvfs->f_flag |= ST_RDONLY;

    stvfs->f_ffree = 0;

    /*
    https://stackoverflow.com/questions/54823541/what-do-f-bsize-and-f-frsize-in-struct-statvfs-stand-for
    */
    stvfs->f_bsize  =  // 512;                   /* Filesystem block size */
    stvfs->f_frsize = vol->datablockSize;        /* Fragment size */

    stvfs->f_blocks =                            /* Size of fs in f_frsize units */
        (unsigned) ( vol->lastBlock - vol->firstBlock - 2 );
    stvfs->f_bfree =                             /* Number of free blocks */
    stvfs->f_bavail = blocks_free;               /* Number of free blocks for
                                                    unprivileged users */

    stvfs->f_files   = 1;                         /* Number of inodes */
    stvfs->f_ffree   = 1;                         /* Number of free inodes */
    stvfs->f_favail  = 1;                         /* Number of free inodes for
                                                     unprivileged users */
    //stvfs->f_fsid    = 0;                         /* Filesystem ID */
    //stvfs->f_flag    = 0x00000002;              /* Mount flags */
    stvfs->f_namemax = 30;                        /* Maximum filename length */

#ifdef DEBUG_ADFFS
    adffs_log_statvfs( stvfs );
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
    adffs_log_info ( "\nadffs_getattr (\n"
                     "    path    = \"%s\",\n"
                     "    statbuf = 0x%" PRIxPTR " )\n",
          path, statbuf );
#endif

    memset ( statbuf, 0, sizeof ( *statbuf ) );

    const char * path_relative = path;

    // skip all leading '/' from the path
    // (normally, fuse always starts with a single '/')
    while ( *path_relative == '/' )
        path_relative++;

    adfimage_t * const adfimage = fs_state->adfimage;
    adfimage_dentry_t dentry;
    if ( *path_relative == '\0' ) {
        // main directory

        dentry = adfimage_get_root_dentry ( adfimage );

        /* root dir accees permissions never seem to be set properly...
           (if translated - nothing will be accesible) */
        /*const int perms = adfimage_getperm( &dentry );
        statbuf->st_mode = S_IFDIR |
                ( perms & ADF_PERM_READ    ? S_IRUSR | S_IRGRP | S_IROTH : 0 ) |
                ( perms & ADF_PERM_WRITE   ? S_IWUSR : 0 ) |
                //( perms & ADF_PERM_EXECUTE ? S_IXUSR | S_IXGRP | S_IXOTH : 0 );
                S_IXUSR | S_IXGRP | S_IXOTH;  // executable (entering dir.) for all
        */
        /* setting reasonable defaults instead */
        statbuf->st_mode = S_IFDIR |
            S_IRUSR | S_IXUSR | S_IWUSR |
            S_IRGRP | S_IXGRP |
            S_IROTH | S_IXOTH;

        //statbuf->st_size = dentry.adflib_entry.size;   // always 0 for directories(?)
                                                         // (to improve in ADFlib?)
        statbuf->st_size = adfimage_count_cwd_entries ( adfimage );

        // links count - always 1 (what should it be?)
        statbuf->st_nlink = 1;

    } else {         // path is a non-empty string
                     // so anything besides the main directory

        struct AdfVolume * const vol = adfimage->vol;

        // first, find and enter the directory where is dir. entry to check
        char * dirpath_buf = strdup ( path );
        char * dir_path = dirname ( dirpath_buf );

#ifdef DEBUG_ADFFS
        adffs_log_info ( "adffs_getattr(): Entering directory the directory %s.\n",
                         dir_path );
#endif
        if ( ! adfimage_chdir ( adfimage, dir_path ) ) {
            adffs_log_info ( "adffs_getattr(): Cannot chdir to the directory %s.\n",
                             dir_path );
            return -ENOENT;
        }
        free ( dirpath_buf );

#ifdef DEBUG_ADFFS
        adffs_log_info ( "adffs_getattr(): Current directory: %s.\n",
                         adfimage_getcwd ( adfimage ) );
#endif
        char * direntry_buf = strdup ( path );
        char * direntry_name = basename ( direntry_buf );

#ifdef DEBUG_ADFFS
        adffs_log_info ( "adffs_getattr(): direntry name: %s.\n",
                         direntry_name );
#endif
        if ( *direntry_name == '\0' ) {
            // empty name means that given path is a directory
            // to which we entered above - so we need to check
            // properties of the current directory
            direntry_name = ".";
        }

        dentry = adfimage_getdentry ( adfimage, direntry_name );

        if ( dentry.type == ADFVOLUME_DENTRY_FILE ||
             dentry.type == ADFVOLUME_DENTRY_LINKFILE )
        {
            const int perms = adfimage_getperm( &dentry );
            statbuf->st_mode = S_IFREG |
                ( perms & ADF_PERM_READ    ? S_IRUSR | S_IRGRP | S_IROTH : 0 ) |
                ( perms & ADF_PERM_WRITE   ? S_IWUSR : 0 ) |
                ( perms & ADF_PERM_EXECUTE ? S_IXUSR | S_IXGRP | S_IXOTH : 0 );
            statbuf->st_nlink = 1;

            struct AdfFile * afile = adfFileOpen ( vol, direntry_name,
                                                   ADF_FILE_MODE_READ );
            if ( afile ) {
                statbuf->st_size = afile->fileHdr->byteSize;
                statbuf->st_blocks = statbuf->st_size / 512 + 1;
            } else {
                adffs_log_info ( "adffs_getattr(): Error opening file: %s\n", path );
            }
            adfFileClose ( afile );

        } else if ( dentry.type == ADFVOLUME_DENTRY_DIRECTORY ||
                    dentry.type == ADFVOLUME_DENTRY_LINKDIR )
        {
            const int perms = adfimage_getperm( &dentry );
            statbuf->st_mode = S_IFDIR |
                ( perms & ADF_PERM_READ    ? S_IRUSR | S_IRGRP | S_IROTH : 0 ) |
                ( perms & ADF_PERM_WRITE   ? S_IWUSR : 0 ) |
                //( perms & ADF_PERM_EXECUTE ? S_IXUSR | S_IXGRP | S_IXOTH : 0 );
                S_IXUSR | S_IXGRP | S_IXOTH;  // executable (entering dir.) for all

            //statbuf->st_size = dentry.adflib_entry.size;   // always 0 for directories(?)
                                                             // (to improve in ADFlib?)
            statbuf->st_size = adfimage_count_dir_entries ( adfimage, path );
            statbuf->st_nlink = 1;

        } else if ( dentry.type == ADFVOLUME_DENTRY_SOFTLINK ) {
            statbuf->st_mode = S_IFLNK |
                S_IRUSR | S_IXUSR |
                S_IRGRP | S_IXGRP |
                S_IROTH | S_IXOTH;
            statbuf->st_nlink = 1;

        } else if ( dentry.type == ADFVOLUME_DENTRY_UNKNOWN ) {
            adffs_log_info ( "adffs_getattr(): Unknown dir. entry: %s, "
                             "adflib type: %d\n",
                             path, dentry.adflib_entry.type );
        } else {
            // file/dirname not found
            free ( direntry_buf );
            adfToRootDir ( vol );
            return -ENOENT;
        }

        free ( direntry_buf );
        adfToRootDir ( vol );
    }

    statbuf->st_uid = geteuid();
    statbuf->st_gid = getegid();

    statbuf->st_atime =
    statbuf->st_mtime =
    statbuf->st_ctime = localtime_to_time_t ( dentry.adflib_entry.year,
                                              dentry.adflib_entry.month,
                                              dentry.adflib_entry.days,
                                              dentry.adflib_entry.hour,
                                              dentry.adflib_entry.mins,
                                              dentry.adflib_entry.secs );

#ifdef DEBUG_ADFFS
    adffs_log_info ( "\nadffs_getattr time:\n"
                     "    year   = %d\n"
                     "    month  = %d\n"
                     "    day    = %d\n"
                     "    hour   = %d\n"
                     "    min    = %d\n"
                     "    sec    = %d\n"
                     "    time_t = %lld\n\n",
                     dentry.adflib_entry.year,
                     dentry.adflib_entry.month,
                     dentry.adflib_entry.days,
                     dentry.adflib_entry.hour,
                     dentry.adflib_entry.mins,
                     dentry.adflib_entry.secs,
                     (long long) statbuf->st_ctime );
#endif

    statbuf->st_blksize = adfimage->fstat.st_blksize;

#ifdef DEBUG_ADFFS
    adffs_log_stat( statbuf );
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
    adffs_log_info ( "\nadffs_read (\n"
                     "    path   = \"%s\",\n"
                     "    buf    = 0x%" PRIxPTR ",\n"
                     "    size   = %d,\n"
                     "    offset = %lld,\n"
                     "    finfo  = 0x%" PRIxPTR " )\n",
                     path, buffer, size, offset, finfo );
#else
    (void) finfo;
#endif

    int bytes_read = adfimage_read ( fs_state->adfimage, path,
                                     buffer, size, offset );

#ifdef DEBUG_ADFFS
    //adffs_log_info ( fs_state->logfile,
    //           "adffs_read () => %d (%s)\n", bytes_read,
    //           size == (size_t) bytes_read ? "OK" : "READ ERROR" );
#endif

    return bytes_read;
}


int adffs_write ( const char *            path,
                  const char *            buffer,
                  size_t                  size,
                  off_t                   offset,
                  struct fuse_file_info * finfo )
{
    const adffs_state_t * const fs_state =
        ( adffs_state_t * ) fuse_get_context()->private_data;

#ifdef DEBUG_ADFFS
    adffs_log_info ( "\nadffs_write (\n"
                     "    path   = \"%s\",\n"
                     "    buf    = 0x%" PRIxPTR ",\n"
                     "    size   = %d,\n"
                     "    offset = %lld,\n"
                     "    finfo  = 0x%" PRIxPTR " )\n",
                     path, buffer, size, offset, finfo );
#else
    (void) finfo;
#endif

    int bytes_written = adfimage_write ( fs_state->adfimage, path,
                                         ( char * ) buffer, size, offset );

#ifdef DEBUG_ADFFS
    adffs_log_info ( "adffs_write () => %d (%s)\n", bytes_written,
                     size == (size_t) bytes_written ? "OK" : "WRITE ERROR" );
#endif

    return bytes_written;
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
    adffs_log_info ( "\nadffs_readdir (\n"
                     "    path   = \"%s\",\n"
                     "    buf    = 0x%" PRIxPTR ",\n"
                     "    filler = 0x%"  PRIxPTR ",\n"
                     "    offset = %lld,\n"
                     "    finfo  = 0x%" PRIxPTR " )\n",
                     path, buffer, filler, offset, finfo );
#else
    (void) offset;  (void) finfo;
#endif
    adfimage_t * const adfimage = fs_state->adfimage;
    struct AdfVolume * const vol = adfimage->vol;
    if ( ! adfimage_chdir ( adfimage, path ) ) {
        adffs_log_info ( "adffs_read(): Cannot chdir to the directory %s.\n",
                         path );
        adfToRootDir ( vol );
        return -ENOENT;
    }
    
    filler ( buffer, ".", NULL, 0 );
    filler ( buffer, "..", NULL, 0 );

    struct AdfList * const dentries = adfGetDirEnt ( vol, vol->curDirPtr );
    /*if ( ! dentries ) {
        adffs_log_info ( fs_state->logfile,
                   "adfimage_getdentry(): Error getting dir entries,"
                   "path %s\n", path );
        return -ENOENT; //?
    }*/

    for ( struct AdfList * lentry = dentries ;
          lentry ;
          lentry = lentry->next )
    {
        struct AdfEntry * const dentry =
            ( struct AdfEntry * ) lentry->content;
        //printf (" type: %d, size: %d, name: %s, comment: %s\n",
        //        dentry->type,
        //        dentry->size,
        //        dentry->name,
        //        dentry->comment );
        //printEntry(struct Entry* entry);
        //printEntry ( dentry );    
        if ( filler ( buffer, dentry->name, NULL, 0 ) ) {
            adffs_log_info ( "adffs_readdir: filler: buffer full\n" );
            adfToRootDir ( vol );
            return -EAGAIN; // check what error return in such case(!)
                            // probably something from /usr/include/asm-generic/errno-base.h (?)
        }
    }

#ifdef DEBUG_ADFFS
    adffs_log_fuse_file_info( finfo );
#endif

    adfToRootDir ( vol );
    return 0;
}


int adffs_readlink ( const char * path,
                     char *       buf,
		     size_t       len )
{
    const adffs_state_t * const fs_state =
        ( adffs_state_t * ) fuse_get_context()->private_data;
#ifdef DEBUG_ADFFS
    adffs_log_info ( "\nadffs_readlink (\n"
                     "    path = \"%s\",\n"
                     "    buf  = 0x%" PRIxPTR ",\n"
                     "    len  = %lld,\n",
                     path, buf, len );
#endif
    int status = adfimage_readlink ( fs_state->adfimage, path, buf, len );

#ifdef DEBUG_ADFFS
    adffs_log_info ( "\nadffs_readlink:  buf  = %s, status %d\n",
                     buf, status );
#endif
    //strncpy ( buf, "secret.S", len );

    return status;
}


int adffs_mkdir ( const char * dirpath,
                  mode_t       mode )
{
    const adffs_state_t * const fs_state =
        ( adffs_state_t * ) fuse_get_context()->private_data;

#ifdef DEBUG_ADFFS
    adffs_log_info ( "\nadffs_mkdir (\n"
                     "    dirpath = \"%s\",\n"
                     //"    mode    = 0x%" PRIxPTR ",\n"
                     "    mode    = %lld )\n",
                     dirpath, mode );
#endif

    int status = adfimage_mkdir ( fs_state->adfimage, dirpath, mode );

    return status;
}


int adffs_rmdir ( const char * dirpath )
{
    const adffs_state_t * const fs_state =
        ( adffs_state_t * ) fuse_get_context()->private_data;

#ifdef DEBUG_ADFFS
    adffs_log_info ( "\nadffs_rmdir (\n"
                     "    dirpath = \"%s\",\n",
                     dirpath );
#endif

    int status = adfimage_rmdir ( fs_state->adfimage, dirpath );

    return status;
}


int adffs_create ( const char *           filepath,
                   mode_t                 mode,
                   struct fuse_file_info *finfo )
{
    (void) finfo;
    const adffs_state_t * const fs_state =
        ( adffs_state_t * ) fuse_get_context()->private_data;

#ifdef DEBUG_ADFFS
    adffs_log_info ( "\nadffs_create (\n"
                     "    filepath = \"%s\",\n"
                     "    mode     = %lld )\n",
                     filepath, mode );
#endif

    return adfimage_create ( fs_state->adfimage, filepath, mode );
}

int adffs_unlink ( const char * filepath )
{
   const adffs_state_t * const fs_state =
        ( adffs_state_t * ) fuse_get_context()->private_data;

#ifdef DEBUG_ADFFS
    adffs_log_info ( "\nadffs_unlink (\n"
                     "    filepath = \"%s\",\n",
                     filepath );
#endif

    int status = adfimage_unlink ( fs_state->adfimage, filepath );

    return status;
}



int adffs_open ( const char *            filepath,
                 struct fuse_file_info * finfo )
{
    (void) finfo;
    const adffs_state_t * const fs_state =
        ( adffs_state_t * ) fuse_get_context()->private_data;

#ifdef DEBUG_ADFFS
    adffs_log_info ( "\nadffs_open (\n"
                     "    filepath = \"%s\",\n",
                     filepath );
#endif

    struct AdfFile * file = adfimage_file_open ( fs_state->adfimage,
                                                 filepath, ADF_FILE_MODE_READ );
    int status = ( file != NULL ) ? 0 : -1;
    adfimage_file_close ( file );

    return status;
}


int adffs_chmod ( const char * path,
                  mode_t       mode )
{
    const adffs_state_t * const fs_state =
        ( adffs_state_t * ) fuse_get_context()->private_data;
#ifdef DEBUG_ADFFS
    adffs_log_info ( "\nadffs_chmod (\n"
                     "    path = \"%s\", mode = %o\n",
                     path, mode );
#else
    (void) path, (void) mode;
#endif

    // only user permissions are managed (not touching group/other)
    int perms =
        ( mode & S_IRUSR ? ADF_PERM_READ    : 0 ) |
        ( mode & S_IWUSR ? ADF_PERM_WRITE   : 0 ) |
        ( mode & S_IXUSR ? ADF_PERM_EXECUTE : 0 );

    if ( ! adfimage_setperm( fs_state->adfimage, path, perms ) ) {
#ifdef DEBUG_ADFFS
        adffs_log_info( "\nadffs_chmod: error setting permissions\n" );
#endif
        return EINVAL;   // a better error here?
    }

#ifdef DEBUG_ADFFS
    adffs_log_info( "\nadffs_chmod: permissions set\n" );
#endif

    return 0;
}


int adffs_chown ( const char * path,
                  uid_t        uid,
                  gid_t        gid )
{
#ifdef DEBUG_ADFFS
    const adffs_state_t * const fs_state =
        ( adffs_state_t * ) fuse_get_context()->private_data;
    adffs_log_info ( "\nadffs_chown (\n"
                     "    path = \"%s\", uid = %u, gid = %u\n",
                     path, uid, gid );
#else
    (void) path, (void) uid, (void) gid;
#endif
    return 0;
}

/** Change the size of a file */
int adffs_truncate ( const char * path,
                     off_t        new_size )
{
    const adffs_state_t * const fs_state =
        ( adffs_state_t * ) fuse_get_context()->private_data;

#ifdef DEBUG_ADFFS
    adffs_log_info ( "\nadffs_truncate (\n"
                     "    filepath = \"%s\", size = %lu )\n",
                     path, new_size );
#endif
    int status = adfimage_file_truncate ( fs_state->adfimage, path,
                                          (long unsigned) new_size );
    return ( status == 0 ? 0 : -1 );
}


int adffs_rename ( const char * src_path,
                   const char * dst_path )
{
    const adffs_state_t * const fs_state =
        ( adffs_state_t * ) fuse_get_context()->private_data;

#ifdef DEBUG_ADFFS
    adffs_log_info ( "\nadffs_rename (\n"
                     "    src_path = \"%s\", dst_path = \"%s\" )\n",
                     src_path, dst_path );
#endif
    return adfimage_file_rename ( fs_state->adfimage, src_path, dst_path );
}


//int (*utimens) (const char *, const struct timespec tv[2]);
int adffs_utimens ( const char *          path,
                    const struct timespec tv[2] )
{
#ifdef DEBUG_ADFFS
    const adffs_state_t * const fs_state =
        ( adffs_state_t * ) fuse_get_context()->private_data;
    adffs_log_info ( "\nadffs_utimens (\n"
                     "    path = \"%s\", timespec[0] = %u, timespec[1] = %u )\n",
                     path, tv[0], tv[1] );
#else
    (void) path, (void) tv;
#endif
    return 0;
}

// struct fuse_operations: /usr/include/fuse/fuse.h
struct fuse_operations adffs_oper = {
    .getattr    = adffs_getattr,
    .readlink   = adffs_readlink,
    .getdir     = NULL,       // deprecated
    .mknod      = NULL,
    .mkdir      = adffs_mkdir,
    .unlink     = adffs_unlink,
    .rmdir      = adffs_rmdir,
    .symlink    = NULL,
    .rename     = adffs_rename,
    .link       = NULL,
    .chmod      = adffs_chmod,
    .chown      = adffs_chown,
    .truncate   = adffs_truncate,
    .utime      = NULL,
    .open       = adffs_open,
    .read       = adffs_read,
    .write      = adffs_write,
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
    .create     = adffs_create,
    .ftruncate  = NULL,
    .fgetattr   = NULL,
    .lock       = NULL,
    .utimens    = adffs_utimens,
    .bmap       = NULL,
    .ioctl      = NULL,
    .poll       = NULL,
    .write_buf  = NULL,
    .read_buf   = NULL,
    .flock      = NULL,
    .fallocate  = NULL
};
