
#include "adfimage.h"

#include "adffs_log.h"

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
                const bool   read_only );

static struct AdfVolume *
    mount_volume ( struct AdfDevice * const dev,
                   unsigned int          partition,
                   bool                  read_only );

static void append_dir ( adfimage_t * const adfimage,
                         const char * const dir );

static bool isBlockAllocationBitmapValid ( struct AdfVolume * const vol );


adfimage_t * adfimage_open ( char * const filename,
                             unsigned int volume,
                             bool         read_only,
                             const bool   ignore_checksum_errors )
{
    if ( adfLibInit() != ADF_RC_OK )
        return NULL;

    adfEnvSetFct ( adffs_log_info, adffs_log_info, adffs_log_info, NULL );
    adfEnvSetProperty ( ADF_PR_IGNORE_CHECKSUM_ERRORS, ignore_checksum_errors );

    struct AdfDevice * const dev = mount_dev ( filename, read_only );
    if ( ! dev ) {
        goto adfimage_open_error_cleanup_adflib;
    }

    struct AdfVolume * const vol = mount_volume ( dev, volume, read_only );
    if ( ! vol ) {
        goto adfimage_open_error_cleanup_dev;
    }

    if ( ( ! read_only ) &&
         ( ! isBlockAllocationBitmapValid ( vol ) ) )
    {
        adffs_log_info ( "adfimage_open: error: invalid bitmap, "
                         "cannot mount read-write volume %s\n", vol->volName );
        goto adfimage_open_error_cleanup_vol;
    }

    adfimage_t * const adfimage = malloc ( sizeof ( adfimage_t ) );
    if ( ! adfimage ) {
        adffs_log_info ( "adfimage_open: error: Cannot allocate memory for adfimage data\n" );
        goto adfimage_open_error_cleanup_vol;
    }

    adfimage->filename = filename;
    adfimage->dev = dev;
    adfimage->vol = vol;
    strcpy ( adfimage->cwd, "/" );
    
    stat ( adfimage->filename, &adfimage->fstat );

#ifdef DEBUG_ADFIMAGE
    const char
        * const devinfo = adfDevGetInfo( dev ),
        * const volinfo = adfVolGetInfo( dev );
    adffs_log_info( "\n%s: %s\n%s\n", __func__, devinfo, volinfo );
    free( devinfo );
    free( volinfo );
#endif

    return adfimage;

adfimage_open_error_cleanup_vol:
    adfVolUnMount( vol );

adfimage_open_error_cleanup_dev:
    adfDevUnMount( dev );
    adfDevClose( dev );

adfimage_open_error_cleanup_adflib:
    adfLibCleanUp();
    return NULL;
}


void adfimage_close ( adfimage_t ** adfimage )
{
    if ( ! *adfimage )
        return;
    
    // Note: no freeing adfimage->filename
    //       ( as it points to string from argv[] )

    if ( (*adfimage)->vol )
        adfVolUnMount ( (*adfimage)->vol );

    if ( (*adfimage)->dev ) {
        adfDevUnMount ( (*adfimage)->dev );
        adfDevClose ( (*adfimage)->dev );
    }
    
    free ( *adfimage );
    *adfimage = NULL;

    adfLibCleanUp();
}


static int adflist_count_entries ( const struct AdfList * const list )
{
    int nentries = 0;
    for ( const struct AdfList * cell = list;
          cell != NULL;
          cell = cell->next )
    {
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

/*
typedef struct adfimage_array_str_s {
    char **  str;
    unsigned nstr;
} adfimage_array_str;

adfimage_array_str adfimage_get_cwd_entries ( adfimage_t * const adfimage )
{
    struct AdfVolume * const vol = adfimage->vol;
    adfimage_array_str entry_names = { NULL, 0 };
    struct AdfList * list = adfGetDirEnt ( vol, vol->curDirPtr );
    unsigned nentries = adflist_count_entries ( list );
    unsigned i = 0;
    for ( struct AdfList * cell = list ;
          cell != NULL ;
          cell = cell->next, i++ )
    {
        //printEntry ( cell->content );
        struct AdfEntry * const dentry =
            ( struct AdfEntry * ) cell->content;
        entry_names.str[i] = malloc ( strlen ( dentry->name ) + 1 );
        strcpy ( entry_names.str[i], dentry->name );
    }
    adfFreeDirList ( list );
    entry_names.nstr = nentries;
    return entry_names;
}
*/

adfimage_dentry_t adfimage_get_root_dentry ( adfimage_t * const adfimage )
{
    adfimage_dentry_t adf_dentry = {
        .type = ADFVOLUME_DENTRY_NONE
    };

    struct AdfRootBlock rootBlock;
    struct AdfVolume * const vol = adfimage->vol;

    if ( adfReadRootBlock ( vol, (unsigned) vol->rootBlock, &rootBlock ) != ADF_RC_OK )
        return adf_dentry;

    if ( adfEntBlock2Entry ( ( struct AdfEntryBlock * ) &rootBlock,
                             &adf_dentry.adflib_entry ) != ADF_RC_OK )
        return adf_dentry;

    if ( adf_dentry.adflib_entry.type == ADF_ST_ROOT ) {
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
        adffs_log_info ( "adfimage_getdentry(): Error getting dir entries,"
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
        if ( dentry->type == ADF_ST_FILE ) {
            adf_dentry.type = ADFVOLUME_DENTRY_FILE;
        }

        // directory
        else if ( dentry->type == ADF_ST_ROOT ||
                  dentry->type == ADF_ST_DIR )
        {
            adf_dentry.type = ADFVOLUME_DENTRY_DIRECTORY;
        }

        // "hard" file link
        else if ( dentry->type == ADF_ST_LFILE ) {
            adf_dentry.type = ADFVOLUME_DENTRY_LINKFILE;
        }

        // "hard" directory link
        else if ( dentry->type == ADF_ST_LDIR ) {
            adf_dentry.type = ADFVOLUME_DENTRY_LINKDIR;
        }

        // softlink
        else if ( dentry->type == ADF_ST_LSOFT ) {
            adf_dentry.type = ADFVOLUME_DENTRY_SOFTLINK;
        }

        else {
            adffs_log_info ( "adfimage_getdentry(): pathname '%s' has unsupported "
                             "type: %d, \n", pathname, dentry->type );
            adf_dentry.type = ADFVOLUME_DENTRY_UNKNOWN;
        }
    }
    adfListFree ( dentries );

    // go back to the working directory (if necessary)
    if ( cwd ) {
        adfimage_chdir ( adfimage, cwd );
        free ( cwd );
    }

    return adf_dentry;
}

int adfimage_getperm( adfimage_dentry_t * const dentry )
{
    return
        ( adfAccHasR( dentry->adflib_entry.access ) ? 0 : ADF_PERM_READ     ) |
        ( adfAccHasW( dentry->adflib_entry.access ) ? 0 : ADF_PERM_WRITE    ) |
        ( adfAccHasE( dentry->adflib_entry.access ) ? 0 : ADF_PERM_EXECUTE  );
}


bool adfimage_setperm( adfimage_t * const adfimage,
                       const char *       path,
                       const int          perms )
{
    adfimage_dentry_t dentry = adfimage_getdentry( adfimage, path );
    if ( ! adfimage_dentry_valid( &dentry ) )
        return false;

    struct AdfEntryBlock entryBlock;
    if ( adfReadEntryBlock( adfimage->vol, dentry.adflib_entry.sector,
                            &entryBlock ) != ADF_RC_OK )
    {
        return false;
    }

    if ( perms & ADF_PERM_READ )  entryBlock.access &= !ADF_ACCMASK_R;
    else                          entryBlock.access |= ADF_ACCMASK_R;

    if ( perms & ADF_PERM_WRITE )  entryBlock.access &= !ADF_ACCMASK_W;
    else                           entryBlock.access |= ADF_ACCMASK_W;

    if ( perms & ADF_PERM_EXECUTE )  entryBlock.access &= !ADF_ACCMASK_E;
    else                             entryBlock.access |= ADF_ACCMASK_E;

    if ( adfWriteEntryBlock( adfimage->vol, dentry.adflib_entry.sector,
                             &entryBlock ) != ADF_RC_OK )
    {
        return false;
    }

    return true;
}

const char * adfimage_getcwd ( const adfimage_t * const adfimage )
{
    return adfimage->cwd;
}


bool adfimage_chdir ( adfimage_t * const adfimage,
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
        if ( adfChangeDir ( vol, ( char * ) dir ) != ADF_RC_OK ) {
            free ( dir_path );
            return false;
        }
        append_dir ( adfimage, dir );
        dir = dir_end + 1;
    }
    if ( adfChangeDir ( vol, ( char * ) dir ) != ADF_RC_OK ) {
        free ( dir_path );
        return false;
    }
    append_dir ( adfimage, dir );

    free ( dir_path );
    return true;
}


struct AdfFile * adfimage_file_open ( adfimage_t * const adfimage,
                                      const char *       pathstr,
                                      const AdfFileMode  mode )
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
            //adffs_log_info ( "adffs_read(): Cannot chdir to the directory %s.\n",
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
        //adffs_log_info ( "Error opening file: %s\n", path );
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
            //adffs_log_info ( "adffs_read(): Cannot chdir to the directory %s.\n",
            //           dir_path );
            free ( cwd );
            free ( path );
            return -ENOENT;
        }
    }

    // open the file
    struct AdfVolume * const vol = adfimage->vol;
    struct AdfFile * file = adfFileOpen ( vol, path->entryname,
                                          ADF_FILE_MODE_READ );
    free ( path );
    if ( file == NULL ) {
        //adffs_log_info ( "Error opening file: %s\n", path );
        return -ENOENT;
    }

    // seek and read the file
    int32_t bytes_read = 0;
    if ( adfFileSeek ( file, (unsigned) offset ) == ADF_RC_OK ) {
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
            //adffs_log_info ( "adffs_read(): Cannot chdir to the directory %s.\n",
            //           dir_path );
            free ( cwd );
            free ( path );
            return -ENOENT;
        }
    }

    // open the file
    struct AdfVolume * const vol = adfimage->vol;
    struct AdfFile * file = adfFileOpen ( vol, path->entryname,
                                          ADF_FILE_MODE_WRITE );
    free ( path );
    if ( file == NULL ) {
        //adffs_log_info ( "Error opening file: %s\n", path );
        return -ENOENT;  // ?
    }

    // seek and write the file
    int32_t bytes_written = 0;
    if ( adfFileSeek ( file, (unsigned) offset ) == ADF_RC_OK ) {
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
            //adffs_log_info ( "adffs_readlink(): Cannot chdir to the directory %s.\n",
            //           dir_path );
            free ( cwd );
            free ( path );
            return -1;
        }
    }

    // get block of the directory
    struct AdfVolume * const vol = adfimage->vol;
    struct AdfEntryBlock parent;
    if ( adfReadEntryBlock ( vol, vol->curDirPtr, &parent ) != ADF_RC_OK ) {
        status = -1;
        goto readlink_cleanup;
    }

    // get block of the entry concerned (specified with path)
    // ADF_SECTNUM adfNameToEntryBlk(struct Volume *vol, int32_t ht[], char* name,
    //                               struct bEntryBlock *entry, ADF_SECTNUM *nUpdSect)

    //struct bEntryBlock entry;
    struct AdfLinkBlock entry;
    ADF_SECTNUM nUpdSect;
    ADF_SECTNUM sectNum = adfNameToEntryBlk ( vol,
                                              parent.hashTable,
                                              path->entryname,
                                              ( struct AdfEntryBlock * ) &entry,
                                              &nUpdSect );
    if ( sectNum == -1 ) {
        status = -2;
        goto readlink_cleanup;
    }

    memset ( buffer, 0, len_max );
    if ( entry.secType == ADF_ST_LSOFT ) {
        strncpy ( buffer, entry.realName,
                  //entry.name,
                  //min ( len_max - 1, entry.nameLen ) );
                  len_max );
    } else {
        // readlink() is only for symlinks - hardlinks are just open()
        // so normally this execution path should never happen
        //assert (false);

        // hardlinks: ADF_ST_LFILE, ADF_ST_LDIR
        if ( adfReadEntryBlock ( vol, entry.realEntry,
                                 //entry.nextLink,
                                 ( struct AdfEntryBlock * ) &entry ) != ADF_RC_OK )
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

    //ADF_RETCODE adfCreateDir(struct Volume* vol, ADF_SECTNUM nParent, char* name);
    ADF_RETCODE status = adfCreateDir ( vol, vol->curDirPtr, ( char * ) dir_name );

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

    //ADF_RETCODE adfRemoveEntry(struct Volume *vol, ADF_SECTNUM pSect, char *name)
    ADF_RETCODE status = adfRemoveEntry ( vol, vol->curDirPtr, dir_name );

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

    //ADF_RETCODE adfCreateDir(struct Volume* vol, ADF_SECTNUM nParent, char* name);
    struct AdfFileHeaderBlock fhdr;
    int status = ( adfCreateFile ( vol, vol->curDirPtr,
                                   ( char * ) file_name, &fhdr ) == ADF_RC_OK ) ?
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
    struct AdfFile * const file = adfimage_file_open ( adfimage, path,
                                                       ADF_FILE_MODE_WRITE );
    if ( ! file ) {
        //adffs_log_info ( "Error opening file: %s\n", path );
        return -ENOENT;
    }

    ADF_RETCODE rc = adfFileTruncate ( file, (unsigned) new_size );
    adfimage_file_close ( file );

    return ( rc == ADF_RC_OK ? 0 : -1 );
}


// return value: 0 on success, != 0 on error
int adfimage_file_rename ( adfimage_t * const adfimage,
                           const char * const src_pathstr,
                           const char * const dst_pathstr )
{

#ifdef DEBUG_ADFIMAGE
    if ( src_pathstr == NULL || dst_pathstr == NULL )
        return -EINVAL;
    adffs_log_info ( "adfimage_file_rename (. '%s',  '%s')\n",
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
    adffs_log_info ( "adfimage_file_rename,   src: dir '%s', entry '%s'\n"
                     "                        dst: dir '%s', entry '%s'\n",
                     src_path->dirpath, src_path->entryname,
                     dst_path->dirpath, dst_path->entryname );
#endif

    // convert (FUSE to AmigaDOS) and sanitize paths
    pathstr_fuse2amigados ( src_path->dirpath );
    pathstr_fuse2amigados ( dst_path->dirpath );

#ifdef DEBUG_ADFIMAGE
    adffs_log_info ( "adfimage_file_rename 2, src: dir '%s', entry '%s'\n"
                     "                        dst: dir '%s', entry '%s'\n",
                     src_path->dirpath, src_path->entryname,
                     dst_path->dirpath, dst_path->entryname );
#endif

    // get and check parent entry for source
    adfimage_dentry_t src_parent_entry =
        adfimage_getdentry ( adfimage, src_path->dirpath );

    if ( src_parent_entry.type == ADFVOLUME_DENTRY_NONE ) {
        adffs_log_info ( "adfimage_file_rename: no such dir path: '%s'\n",
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

    ADF_SECTNUM src_parent_sector =
        ( src_parent_entry.type == ADFVOLUME_DENTRY_DIRECTORY ?
          src_parent_entry.adflib_entry.sector :
          // ( src_parent_entry.type == ADFVOLUME_DENTRY_LINKDIR )
          src_parent_entry.adflib_entry.real );
    if ( src_parent_sector < 2 ) {
        adffs_log_info ( "adfimage_file_rename: invalid src parent sector: %d\n",
                         src_parent_sector );
        return -1;
    }
    //assert ( src_parent_sector > 1 );   // not in bootblock...

    ADF_SECTNUM dst_parent_sector =
        ( dst_parent_entry.type == ADFVOLUME_DENTRY_DIRECTORY ?
          dst_parent_entry.adflib_entry.sector :
          // ( dst_parent_entry.type == ADFVOLUME_DENTRY_LINKDIR )
          dst_parent_entry.adflib_entry.real );
    if ( dst_parent_sector < 2 ) {
        adffs_log_info ( "adfimage_file_rename: invalid dst parent sector: %d\n"
                         "dst_parent_entry.adflib_entry.sector %d\n"
                         "dst_parent_entry.adflib_entry.real %d\n",
                         dst_parent_sector,
                         dst_parent_entry.adflib_entry.sector,
                         dst_parent_entry.adflib_entry.real );
        return -1;
    }
    //assert ( dst_parent_sector > 1 );   // not in bootblock...

#ifdef DEBUG_ADFIMAGE
    adffs_log_info ( "adfimage_file_rename: calling adfRenameEntry (\n"
                     "   ... , old parent sect: %d\n"
                     "         old name       : %s\n"
                     "   ... , new parent sect: %d\n"
                     "         new name       : %s\n",
                     src_parent_sector, src_path->entryname,
                     dst_parent_sector, dst_path->entryname );
#endif

    ADF_RETCODE rc = adfRenameEntry ( adfimage->vol,
                                      src_parent_sector, src_path->entryname,
                                      dst_parent_sector, dst_path->entryname );
#ifdef DEBUG_ADFIMAGE
    adffs_log_info ( "adfimage_file_rename: adfRenameEntry() => %d\n", rc );
#endif
    return ( rc == ADF_RC_OK ? 0 : -1 );
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
                const bool   read_only )
{
    // mount device (ie. image file)
#ifdef DEBUG_ADFIMAGE
    printf ("Mounting file: %s\n", adf_filename );
#endif

    struct AdfDevice * const dev =
        adfDevOpen ( adf_filename,
                     read_only ? ADF_ACCESS_MODE_READONLY :
                                 ADF_ACCESS_MODE_READWRITE );
    if ( dev == NULL ) {
        fprintf ( stderr, "Error opening file/device '%s'\n",
                    adf_filename );
        return NULL;
    }

    ADF_RETCODE rc = adfDevMount ( dev );
    if ( rc != ADF_RC_OK ) {
        fprintf ( stderr, "Error mounting file/device %s\n", adf_filename );
        adfDevClose ( dev );
        return NULL;
    }

#ifdef DEBUG_ADFIMAGE
    const char
        * const devinfo = adfDevGetInfo( dev ),
        * const volinfo = adfVolGetInfo( dev );
    adffs_log_info( "\n%s: Mounted device info:\n%s\n%s\n",
                    __func__, devinfo, volinfo );
    free( devinfo );
    free( volinfo );

#endif

    return dev;
}

struct AdfVolume *
    mount_volume ( struct AdfDevice * const dev,
                   unsigned int             partition,
                   bool                     read_only )
{
    // mount volume (volume/partition number, for floppies always 0 (?))
#ifdef DEBUG_ADFIMAGE
    printf ("\nMounting volume (partition) %d\n", partition );
#endif
    struct AdfVolume * const vol =
        adfVolMount ( dev, (int) partition,
                      read_only ? ADF_ACCESS_MODE_READONLY :
                                  ADF_ACCESS_MODE_READWRITE );
    if ( vol == NULL ) {
        fprintf ( stderr,  "Error opening volume %d.\n", partition );
        return NULL;
    }

#ifdef DEBUG_ADFIMAGE
    adfVolInfo ( vol );
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

static bool isBlockAllocationBitmapValid ( struct AdfVolume * const vol )
{
    struct AdfRootBlock root;
    //printf ("reading root block from %u\n", vol->rootBlock );
    ADF_RETCODE rc = adfReadRootBlock ( vol, (uint32_t) vol->rootBlock, &root );
    if ( rc != ADF_RC_OK ) {
        adfEnv.eFct ( "Invalid RootBlock, sector %u - aborting...",
                      vol->rootBlock );
        return rc;
    }
    //printf ("root block read, name %s\n", root.diskName );
    return ( root.bmFlag == ADF_BM_VALID );
}
