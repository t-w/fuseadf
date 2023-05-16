
#include "adfimage.h"

#include "log.h"

#include <adf_blk.h>
#include <adf_dir.h>
#include <adf_raw.h>
#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

//#define DEBUG_ADFIMAGE 1

static struct AdfDevice *
    mount_dev ( char * const adf_filename,
                const BOOL   read_only );

static struct AdfVolume *
    mount_volume ( struct AdfDevice * const dev,
                   unsigned int          partition,
                   BOOL                  read_only );

static long getFileSize ( const char * const filename );

static void append_dir ( adfimage_t * const adfimage,
                         const char * const dir );


adfimage_t * adfimage_open ( char * const filename,
                             unsigned int volume,
                             bool         read_only,
                             FILE *       log )
{
    adfEnvInitDefault();

#ifdef DEBUG_ADFIMAGE
    show_version_info();
#endif

    struct AdfDevice * const dev = mount_dev ( filename, read_only );
    if ( ! dev ) {
        adfEnvCleanUp();
        return NULL;
    }

    struct AdfVolume * const vol = mount_volume ( dev, volume, read_only );
    if ( ! vol ) {
        adfUnMountDev ( dev );
        adfEnvCleanUp();
        return NULL;
    }

    adfimage_t * const adfimage = malloc ( sizeof ( adfimage_t ) );
    if ( ! adfimage ) {
        log_info ( log, "adfimage_open: error: Cannot allocate memory for adfimage data\n" );
        adfUnMount ( vol );
        adfUnMountDev ( dev );
        adfEnvCleanUp();
        return NULL;
    }

    adfimage->filename = filename;
    adfimage->size = dev->size;
    adfimage->dev = dev;
    adfimage->vol = vol;
    strcpy ( adfimage->cwd, "/" );
    
    stat ( adfimage->filename, &adfimage->fstat );

#ifdef DEBUG_ADFIMAGE
    log_info ( log, "\nfile size: %ld\n", adfimage->size );
#endif

    adfimage->logfile = log;
    return adfimage;
}


void adfimage_close ( adfimage_t ** adfimage )
{
    if ( ! *adfimage )
        return;
    
    // Note: no freeing adfimage->filename
    //       ( as it points to string from argv[] )

    if ( (*adfimage)->vol )
        adfUnMount ( (*adfimage)->vol );

    if ( (*adfimage)->dev )
        adfUnMountDev ( (*adfimage)->dev );
    
    free ( *adfimage );
    *adfimage = NULL;

    adfEnvCleanUp();
}


static int adflist_count_entries ( const struct AdfList * const list )
{
    assert ( list != NULL );
    const struct AdfList * cell = list;
    int nentries = 0;
    while ( cell ) {
        //printEntry ( cell->content );
        cell = cell->next;
        nentries++;
    }
    return nentries;
}


int adfimage_count_cwd_entries ( adfimage_t * const adfimage )
{
    struct AdfVolume * const vol = adfimage->vol;
    struct AdfList * const list = adfGetDirEnt ( vol, vol->curDirPtr );
    int nentries = adflist_count_entries ( list );
    adfFreeDirList ( list );
    return nentries;
}


int adfimage_count_dir_entries ( adfimage_t * const adfimage,
                                 const char * const dirpath )
{
    char cwd [ ADFIMAGE_MAX_PATH ];
    strcpy ( cwd, adfimage->cwd );
    adfimage_chdir ( adfimage, dirpath );
    int nentries = adfimage_count_cwd_entries ( adfimage );
    adfimage_chdir ( adfimage, cwd );
    return nentries;
}


adfimage_dentry_t adfimage_get_root_dentry ( adfimage_t * const adfimage )
{
    adfimage_dentry_t adf_dentry = {
        .type = ADFVOLUME_DENTRY_NONE
    };

    struct bRootBlock rootBlock;
    struct AdfVolume * const vol = adfimage->vol;

    if ( adfReadRootBlock ( vol, (unsigned) vol->rootBlock, &rootBlock ) != RC_OK )
        return adf_dentry;

    if ( adfEntBlock2Entry ( ( struct bEntryBlock * ) &rootBlock,
                             &adf_dentry.adflib_entry ) != RC_OK )
        return adf_dentry;

    if ( adf_dentry.adflib_entry.type == ST_ROOT ) {
        adf_dentry.type = ADFVOLUME_DENTRY_DIRECTORY;
        adf_dentry.adflib_entry.sector = vol->rootBlock;
        adf_dentry.adflib_entry.real   = 0;
        adf_dentry.adflib_entry.parent = 0;
    }

    return adf_dentry;
}


static struct AdfEntry * adflib_list_find ( struct AdfList * const dentries,
                                            const char * const  entry_name )
{
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

        if ( strcmp ( dentry->name, entry_name ) == 0 ) {
            return dentry;
        }
    }
    return NULL;
}


adfimage_dentry_t adfimage_getdentry ( adfimage_t * const adfimage,
                                       const char * const pathname )
{
    assert ( adfimage != NULL );
    assert ( pathname != NULL );

    // special case first - root directory / entry
    if ( strcmp ( pathname, "/" ) == 0 )
        //|| strcmp ( pathname, "" ) == 0 )
    {
        return adfimage_get_root_dentry ( adfimage );
    }

    adfimage_dentry_t adf_dentry = {
        .type = ADFVOLUME_DENTRY_NONE
    };

    path_t * path = path_create ( pathname );
    if ( path == NULL )
        return adf_dentry;

    // change the directory first (if necessary)
    char * cwd = NULL;
    if ( strlen ( path->dirpath ) > 0 ) {
        cwd = strdup ( adfimage->cwd );
        adfimage_chdir ( adfimage, path->dirpath );
    }

    struct AdfVolume * const vol = adfimage->vol;

    // get directory list entries (for current directory)
    struct AdfList * const dentries = adfGetDirEnt ( vol, vol->curDirPtr );
    if ( ! dentries ) {
        log_info ( adfimage->logfile, "adfimage_getdentry(): Error getting dir entries,"
                   "filename %s\n", pathname );
        free ( path );
        return adf_dentry;
    }

    // find entry in the list
    struct AdfEntry * const dentry = adflib_list_find ( dentries, path->entryname );
    free ( path );

    if ( dentry ) {
        adf_dentry.adflib_entry = *dentry;

        // regular file
        if ( dentry->type == ST_FILE ) {
            adf_dentry.type = ADFVOLUME_DENTRY_FILE;
        }

        // directory
        else if ( dentry->type == ST_ROOT ||
                  dentry->type == ST_DIR )
        {
            adf_dentry.type = ADFVOLUME_DENTRY_DIRECTORY;
        }

        // "hard" file link
        else if ( dentry->type == ST_LFILE ) {
            adf_dentry.type = ADFVOLUME_DENTRY_LINKFILE;
        }

        // "hard" directory link
        else if ( dentry->type == ST_LDIR ) {
            adf_dentry.type = ADFVOLUME_DENTRY_LINKDIR;
        }

        // softlink
        else if ( dentry->type == ST_LSOFT ) {
            adf_dentry.type = ADFVOLUME_DENTRY_SOFTLINK;
        }

        else {
            log_info ( adfimage->logfile,
                       "adfimage_getdentry(): pathname '%s' has unsupported type: %d, \n",
                       pathname, dentry->type );
            adf_dentry.type = ADFVOLUME_DENTRY_UNKNOWN;
        }
    }
    freeList ( dentries );

    // go back to the working directory (if necessary)
    if ( cwd ) {
        adfimage_chdir ( adfimage, cwd );
        free ( cwd );
    }

    return adf_dentry;
}


const char * adfimage_getcwd ( const adfimage_t * const adfimage )
{
    return adfimage->cwd;
}


BOOL adfimage_chdir ( adfimage_t * const adfimage,
                      const char *       path )
{
    struct AdfVolume * const vol = adfimage->vol;

    if ( ! vol || ! path || strlen ( path ) < 1 )
        return false;

    if ( *path == '/' ) {
        adfToRootDir ( vol );
        while ( *path == '/' ) // skip all leading '/' from the path
            path++;
        strcpy ( adfimage->cwd, "/" );
    }

    if ( *path == '\0' || ( strcmp ( path, "." ) == 0 ) )
        // no need to chdir(".")
        return true;

    char * dir_path = strdup ( path );
    char * dir = dir_path;
    char * dir_end;
    while ( *dir && ( dir_end = strchr ( dir, '/' ) ) ) {
        *dir_end = '\0';
        if ( adfChangeDir ( vol, ( char * ) dir ) != RC_OK ) {
            free ( dir_path );
            return false;
        }
        append_dir ( adfimage, dir );
        dir = dir_end + 1;
    }
    if ( adfChangeDir ( vol, ( char * ) dir ) != RC_OK ) {
        free ( dir_path );
        return false;
    }
    append_dir ( adfimage, dir );

    free ( dir_path );
    return true;
}


struct AdfFile * adfimage_file_open ( adfimage_t * const adfimage,
                                      const char *       pathstr,
                                      const char * const mode )
{
    path_t * path = path_create ( pathstr );
    if ( path == NULL )
        return NULL;

    // if necessary (file path is not in the main dir) - enter the directory
    // with the file to read
    char * cwd = NULL;
    if ( strlen ( path->dirpath ) > 0 ) {
        cwd = strdup ( adfimage->cwd );
        if ( ! adfimage_chdir ( adfimage, path->dirpath ) ) {
            //log_info ( fs_state->logfile, "adffs_read(): Cannot chdir to the directory %s.\n",
            //           dir_path );
            free ( cwd );
            free ( path );
            return NULL;
        }
    }

    // open the file
    struct AdfVolume * const vol = adfimage->vol;
    struct AdfFile * file = adfFileOpen ( vol, path->entryname, mode );
    free ( path );
    if ( file == NULL ) {
        //log_info ( fs_state->logfile,
        //           "Error opening file: %s\n", path );
        return NULL;
    }

    // go back to the working directory (if necessary)
    if ( cwd ) {
        adfimage_chdir ( adfimage, cwd );
        free ( cwd );
    }

    return file;
}


void adfimage_file_close ( struct AdfFile * file )
{
    adfFileClose ( file );
}


int adfimage_read ( adfimage_t * const adfimage,
                    const char *       pathstr,
                    char *             buffer,
                    size_t             size,
                    off_t              offset )
{
    path_t * path = path_create ( pathstr );
    if ( path == NULL )
        return -EINVAL; //-ENOENT;

    // if necessary (file path is not in the main dir) - enter the directory
    // with the file to read
    char * cwd = NULL;
    if ( strlen ( path->dirpath ) > 0 ) {
        cwd = strdup ( adfimage->cwd );
        if ( ! adfimage_chdir ( adfimage, path->dirpath ) ) {
            //log_info ( fs_state->logfile, "adffs_read(): Cannot chdir to the directory %s.\n",
            //           dir_path );
            free ( cwd );
            free ( path );
            return -ENOENT;
        }
    }

    // open the file
    struct AdfVolume * const vol = adfimage->vol;
    struct AdfFile * file = adfFileOpen ( vol, path->entryname, "r" );
    free ( path );
    if ( file == NULL ) {
        //log_info ( fs_state->logfile,
        //           "Error opening file: %s\n", path );
        return -ENOENT;
    }

    // seek and read the file
    int32_t bytes_read = 0;
    if ( adfFileSeek ( file, (unsigned) offset ) == RC_OK ) {
        bytes_read = (int32_t) adfFileRead ( file, (unsigned) size,
                                             ( unsigned char * ) buffer );
    }

    // ... and close it
    adfFileClose ( file );

    // go back to the working directory (if necessary)
    if ( cwd ) {
        adfimage_chdir ( adfimage, cwd );
        free ( cwd );
    }

    return bytes_read;
}


int adfimage_write ( adfimage_t * const adfimage,
                     const char *       pathstr,
                     char *             buffer,
                     size_t             size,
                     off_t              offset )
{
    path_t * path = path_create ( pathstr );
    if ( path == NULL )
        return -EINVAL; //-ENOENT;

    // if necessary (file path is not in the main dir) - enter the directory
    // with the file to read
    char * cwd = NULL;
    if ( strlen ( path->dirpath ) > 0 ) {
        cwd = strdup ( adfimage->cwd );
        if ( ! adfimage_chdir ( adfimage, path->dirpath ) ) {
            //log_info ( fs_state->logfile, "adffs_read(): Cannot chdir to the directory %s.\n",
            //           dir_path );
            free ( cwd );
            free ( path );
            return -ENOENT;
        }
    }

    // open the file
    struct AdfVolume * const vol = adfimage->vol;
    struct AdfFile * file = adfFileOpen ( vol, path->entryname, "w" );
    free ( path );
    if ( file == NULL ) {
        //log_info ( fs_state->logfile,
        //           "Error opening file: %s\n", path );
        return -ENOENT;  // ?
    }

    // seek and write the file
    int32_t bytes_written = 0;
    if ( adfFileSeek ( file, (unsigned) offset ) == RC_OK ) {
        bytes_written = (int32_t) adfFileWrite ( file, (unsigned) size,
                                                 ( unsigned char * ) buffer );
    }

    // ... and close it
    adfFileClose ( file );

    // go back to the working directory (if necessary)
    if ( cwd ) {
        adfimage_chdir ( adfimage, cwd );
        free ( cwd );
    }

    return bytes_written;
}


// return value: 0 on success, -1 on error
int adfimage_readlink ( adfimage_t * const adfimage,
                        const char *       pathstr,
                        char *             buffer,
                        size_t             len_max )
{
    int status = 0;

    path_t * path = path_create ( pathstr );
    if ( path == NULL )
        return -EINVAL; //-ENOENT;

    // if necessary (file path is not in the main dir) - enter the directory
    // with the file to read
    char * cwd = NULL;
    if ( strlen ( path->dirpath ) > 0 ) {
        cwd = strdup ( adfimage->cwd );
        if ( ! adfimage_chdir ( adfimage, path->dirpath ) ) {
            //log_info ( fs_state->logfile, "adffs_readlink(): Cannot chdir to the directory %s.\n",
            //           dir_path );
            free ( cwd );
            free ( path );
            return -1;
        }
    }

    // get block of the directory
    struct AdfVolume * const vol = adfimage->vol;
    struct bEntryBlock parent;
    if ( adfReadEntryBlock ( vol, vol->curDirPtr, &parent ) != RC_OK ) {
        status = -1;
        goto readlink_cleanup;
    }

    // get block of the entry concerned (specified with path)
    // SECTNUM adfNameToEntryBlk(struct Volume *vol, int32_t ht[], char* name,
    //                           struct bEntryBlock *entry, SECTNUM *nUpdSect)

    //struct bEntryBlock entry;
    struct bLinkBlock entry;
    SECTNUM nUpdSect;
    SECTNUM sectNum = adfNameToEntryBlk ( vol,
                                          parent.hashTable,
                                          path->entryname,
                                          ( struct bEntryBlock * ) &entry,
                                          &nUpdSect );
    if ( sectNum == -1 ) {
        status = -2;
        goto readlink_cleanup;
    }

    memset ( buffer, 0, len_max );
    if ( entry.secType == ST_LSOFT ) {
        strncpy ( buffer, entry.realName,
                  //entry.name,
                  //min ( len_max - 1, entry.nameLen ) );
                  len_max );
    } else {
        // readlink() is only for symlinks - hardlinks are just open()
        // so normally this execution path should never happen
        //assert (false);

        // hardlinks: ST_LFILE, ST_LDIR
        if ( adfReadEntryBlock ( vol, entry.realEntry,
                                 //entry.nextLink,
                                 ( struct bEntryBlock * ) &entry ) != RC_OK )
        {
            status = -3;
            goto readlink_cleanup;
        }
        strncpy ( buffer, //entry.realName,
                  entry.name,
                  //min ( len_max - 1, entry.nameLen ) );
                  len_max );
    }

readlink_cleanup:
    free ( path );
    if ( cwd ) {
        adfimage_chdir ( adfimage, cwd );
        free ( cwd );
    }

    //strncpy ( buffer, "secret.S", len_max );
    return status;
}


// return value: 0 on success, != 0 on error
int adfimage_mkdir ( adfimage_t * const adfimage,
                     const char *       newdirpath,
                     mode_t             mode )
{
    (void) mode;
    const char * path_relative = newdirpath;

    // skip all leading '/' from the path
    // (normally, fuse always starts with a single '/')
    while ( *path_relative == '/' )
        path_relative++;

    // fuse should never request creating root dir - but making sure
    if ( *path_relative == '\0' ) {
        // empty relative path means main directory (could be just '/') -> error
        return -EEXIST; // EPERM / EACCES / EINVAL / ?
    }

    struct AdfVolume * const vol = adfimage->vol;
    adfToRootDir ( vol );

    // first, find and enter the directory where the new should be created
    char * dirpath_buf = strdup ( path_relative );
    char * dir_path = dirname ( dirpath_buf );

    if ( ! adfimage_chdir ( adfimage, dir_path ) ) {
        return -ENOENT;  // ENOTDIR / EINVAL / ?
    }
    free ( dirpath_buf );

    char * dir_name_buf = strdup ( path_relative );
    char * dir_name = basename ( dir_name_buf );

    //RETCODE adfCreateDir(struct Volume* vol, SECTNUM nParent, char* name);
    RETCODE status = adfCreateDir ( vol, vol->curDirPtr, ( char * ) dir_name );

    free ( dir_name_buf );
    adfToRootDir ( vol );

    return status;
}


// return value: 0 on success, != 0 on error
static int adfimage_remove_entry ( adfimage_t * const adfimage,
                                   const char *       rmpath )
{
    const char * path_relative = rmpath;

    // skip all leading '/' from the path
    // (normally, fuse always starts with a single '/')
    while ( *path_relative == '/' )
        path_relative++;

    // fuse should never request removing root dir - but making sure
    if ( *path_relative == '\0' ) {
        // empty relative path means main directory (could be just '/') -> error
        return -EPERM; // EPERM / EACCES / EINVAL / ?
    }

    struct AdfVolume * const vol = adfimage->vol;
    adfToRootDir ( vol );

    // find and enter the directory where is the direntry (directory) to remove
    char * dirpath_buf = strdup ( path_relative );
    char * dir_path = dirname ( dirpath_buf );

    if ( ! adfimage_chdir ( adfimage, dir_path ) ) {
        return -ENOENT;  // ENOTDIR / EINVAL / ?
    }
    free ( dirpath_buf );

    char * dir_name_buf = strdup ( path_relative );
    char * dir_name = basename ( dir_name_buf );

    //RETCODE adfRemoveEntry(struct Volume *vol, SECTNUM pSect, char *name)
    RETCODE status = adfRemoveEntry ( vol, vol->curDirPtr, dir_name );

    free ( dir_name_buf );
    adfToRootDir ( vol );

    return status;
}


// return value: 0 on success, != 0 on error
int adfimage_rmdir ( adfimage_t * const adfimage,
                     const char *       rmdirpath )
{
    // not needed??? it seems fuse does check entry type itself
    //adfimage_dentry_t dentry = adfimage_getdentry ( adfimage, rmdirpath );
    //if ( dentry.type != ADFVOLUME_DENTRY_DIRECTORY )
    //    return -ENOTDIR; // / EINVAL / ?

    return adfimage_remove_entry ( adfimage, rmdirpath );
}


// return value: 0 on success, != 0 on error
int adfimage_unlink ( adfimage_t * const adfimage,
                      const char *       unlinkpath )
{
    // not needed??? it seems fuse does check entry type itself
    //adfimage_dentry_t dentry = adfimage_getdentry ( adfimage, rmdirpath );
    //if ( dentry.type != ADFVOLUME_DENTRY_FILE )
    //    return -ENOTDIR; // / EINVAL / ?

    return adfimage_remove_entry ( adfimage, unlinkpath );
}




// return value: 0 on success, != 0 on error
int adfimage_create ( adfimage_t * const adfimage,
                      const char *       newfilepath,
                      mode_t             mode )
{
    (void) mode;
    const char * const path_relative = pathstr_get_relative ( newfilepath );

    // fuse should never request creating file with empty name - but make sure
    if ( *path_relative == '\0' ) {
        // empty relative path means main directory (could be just '/') -> error
        return -EINVAL; // EEXIST / EPERM / EACCES / EINVAL / ?
    }

    struct AdfVolume * const vol = adfimage->vol;
    adfToRootDir ( vol );

    // first, find and enter the directory where the new should be created
    char * dirpath_buf = strdup ( path_relative );
    char * dir_path = dirname ( dirpath_buf );

    if ( ! adfimage_chdir ( adfimage, dir_path ) ) {
        free ( dirpath_buf );
        return -ENOENT;  // ENOTDIR / EINVAL / ?
    }
    free ( dirpath_buf );

    char * file_name_buf = strdup ( path_relative );
    char * file_name = basename ( file_name_buf );

    //RETCODE adfCreateDir(struct Volume* vol, SECTNUM nParent, char* name);
    struct bFileHeaderBlock fhdr;
    int status = ( adfCreateFile ( vol, vol->curDirPtr,
                                   ( char * ) file_name, &fhdr ) == RC_OK ) ?
        0 : -1;
    free ( file_name_buf );
    adfToRootDir ( vol );

    return status;
}

// return value: 0 on success, != 0 on error
int adfimage_file_truncate ( adfimage_t * const adfimage,
                             const char *       path,
                             const size_t       new_size )
{
    struct AdfFile * const file = adfimage_file_open ( adfimage, path, "w" );
    if ( ! file ) {
        //log_info ( fs_state->logfile,
        //           "Error opening file: %s\n", path );
        return -ENOENT;
    }

    RETCODE rc = adfFileTruncate ( file, (unsigned) new_size );
    adfimage_file_close ( file );

    return ( rc == RC_OK ? 0 : -1 );
}


// return value: 0 on success, != 0 on error
int adfimage_file_rename ( adfimage_t * const adfimage,
                           const char * const src_pathstr,
                           const char * const dst_pathstr )
{

#ifdef DEBUG_ADFIMAGE
    if ( src_pathstr == NULL || dst_pathstr == NULL )
        return -EINVAL;
    log_info ( adfimage->logfile,
               "adfimage_file_rename (. '%s',  '%s')\n",
               src_pathstr, dst_pathstr );
#endif

    path_t * const src_path = path_create ( src_pathstr );
    path_t * const dst_path = path_create ( dst_pathstr );
    if ( src_path == NULL ||
         dst_path == NULL ||
         ( ! ( path_is_entryname_non_empty ( src_path ) &&
               path_is_entryname_non_empty ( dst_path ) ) ) )
    {
        return -EINVAL;
    }

#ifdef DEBUG_ADFIMAGE
    log_info ( adfimage->logfile,
               "adfimage_file_rename,   src: dir '%s', entry '%s'\n"
               "                        dst: dir '%s', entry '%s'\n",
               src_path->dirpath, src_path->entryname,
               dst_path->dirpath, dst_path->entryname );
#endif

    // convert (FUSE to AmigaDOS) and sanitize paths
    pathstr_fuse2amigados ( src_path->dirpath );
    pathstr_fuse2amigados ( dst_path->dirpath );

#ifdef DEBUG_ADFIMAGE
    log_info ( adfimage->logfile,
               "adfimage_file_rename 2, src: dir '%s', entry '%s'\n"
               "                        dst: dir '%s', entry '%s'\n",
               src_path->dirpath, src_path->entryname,
               dst_path->dirpath, dst_path->entryname );
#endif

    // get and check parent entry for source
    adfimage_dentry_t src_parent_entry =
        adfimage_getdentry ( adfimage, src_path->dirpath );

    if ( src_parent_entry.type == ADFVOLUME_DENTRY_NONE ) {
        log_info ( adfimage->logfile,
                   "adfimage_file_rename: no such dir path: '%s'\n",
                   src_path->dirpath );
        return -ENOENT;
    }

    if ( src_parent_entry.type != ADFVOLUME_DENTRY_DIRECTORY &&
         src_parent_entry.type != ADFVOLUME_DENTRY_LINKDIR )
    {
        return -EINVAL;
    }

    // get and check parent entry for destination
    adfimage_dentry_t dst_parent_entry =
        adfimage_getdentry ( adfimage, dst_path->dirpath );

    if ( dst_parent_entry.type == ADFVOLUME_DENTRY_NONE )
        return -ENOENT;

    if ( dst_parent_entry.type != ADFVOLUME_DENTRY_DIRECTORY &&
         dst_parent_entry.type != ADFVOLUME_DENTRY_LINKDIR )
    {
        return -EINVAL;
    }

    SECTNUM src_parent_sector =
        ( src_parent_entry.type == ADFVOLUME_DENTRY_DIRECTORY ?
          src_parent_entry.adflib_entry.sector :
          // ( src_parent_entry.type == ADFVOLUME_DENTRY_LINKDIR )
          src_parent_entry.adflib_entry.real );
    if ( src_parent_sector < 2 ) {
        log_info ( adfimage->logfile,
                   "adfimage_file_rename: invalid src parent sector: %d\n",
                   src_parent_sector );
        return -1;
    }
    //assert ( src_parent_sector > 1 );   // not in bootblock...

    SECTNUM dst_parent_sector =
        ( dst_parent_entry.type == ADFVOLUME_DENTRY_DIRECTORY ?
          dst_parent_entry.adflib_entry.sector :
          // ( dst_parent_entry.type == ADFVOLUME_DENTRY_LINKDIR )
          dst_parent_entry.adflib_entry.real );
    if ( dst_parent_sector < 2 ) {
        log_info ( adfimage->logfile,
                   "adfimage_file_rename: invalid dst parent sector: %d\n"
                   "dst_parent_entry.adflib_entry.sector %d\n"
                   "dst_parent_entry.adflib_entry.real %d\n",
                   dst_parent_sector,
                   dst_parent_entry.adflib_entry.sector,
                   dst_parent_entry.adflib_entry.real );
        return -1;
    }
    //assert ( dst_parent_sector > 1 );   // not in bootblock...

#ifdef DEBUG_ADFIMAGE
    log_info ( adfimage->logfile,
               "adfimage_file_rename: calling adfRenameEntry (\n"
               "   ... , old parent sect: %d\n"
               "         old name       : %s\n"
               "   ... , new parent sect: %d\n"
               "         new name       : %s\n",
               src_parent_sector, src_path->entryname,
               dst_parent_sector, dst_path->entryname );
#endif

    RETCODE rc = adfRenameEntry ( adfimage->vol,
                                  src_parent_sector, src_path->entryname,
                                  dst_parent_sector, dst_path->entryname );
#ifdef DEBUG_ADFIMAGE
    log_info ( adfimage->logfile,
               "adfimage_file_rename: adfRenameEntry() => %d\n", rc );
#endif
    return ( rc == RC_OK ? 0 : -1 );
}

char * get_adflib_build_version ( void )
{
    return ADFLIB_VERSION;
}

char * get_adflib_build_date ( void )
{
    return ADFLIB_DATE;
}

char * get_adflib_runtime_version ( void )
{
    return adfGetVersionNumber();
}

char * get_adflib_runtime_date ( void )
{
    return adfGetVersionDate();
}


static struct AdfDevice *
    mount_dev ( char * const adf_filename,
                const BOOL   read_only )
{
    // mount device (ie. image file)
#ifdef DEBUG_ADFIMAGE
    printf ("Mounting file: %s\n", adf_filename );
#endif
    struct AdfDevice * const dev = adfMountDev ( adf_filename, read_only );
    if ( dev == NULL ) {
        fprintf ( stderr, "Error opening ADF file: %s\n", adf_filename );
        return NULL;
    }

#ifdef DEBUG_ADFIMAGE
    printf ( "\nMounted device info:\n" );
    //adfDeviceInfo ( dev );
    printf ( "  size:\t\t%d\n  volumes:\t%d\n", dev->size, dev->nVol );
#endif

    return dev;
}

struct AdfVolume *
    mount_volume ( struct AdfDevice * const dev,
                   unsigned int             partition,
                   BOOL                     read_only )
{
    // mount volume (volume/partition number, for floppies always 0 (?))
#ifdef DEBUG_ADFIMAGE
    printf ("\nMounting volume (partition) %d\n", partition );
#endif
    struct AdfVolume * const vol = adfMount ( dev, (int) partition, read_only );
    if ( vol == NULL ) {
        fprintf ( stderr,  "Error opening volume %d.\n", partition );
        return NULL;
    }

#ifdef DEBUG_ADFIMAGE
    adfVolumeInfo ( vol );
#endif
    return vol;
}


static void append_dir ( adfimage_t * const adfimage,
                         const char * const dir )
{
    // update current working dir.
    if ( strcmp ( adfimage->cwd, "/" ) != 0 )
        strcat ( adfimage->cwd, "/" );
    strcat ( adfimage->cwd, dir );
}
