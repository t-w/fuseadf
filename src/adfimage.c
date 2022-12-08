
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

//#define DEBUG_ADFIMAGE 1

static void show_version_info();

static struct Device *
    mount_dev ( char * const adf_filename,
                const BOOL   read_only );

static struct Volume *
    mount_volume ( struct Device * const dev,
                   unsigned int          partition,
                   BOOL                  read_only );

static long getFileSize ( const char * const filename );

static void append_dir ( adfimage_t * const adfimage,
                         const char * const dir );

adfimage_t * adfimage_open ( char * const filename,
                             unsigned int volume )
{

    adfEnvInitDefault();

#ifdef DEBUG_ADFIMAGE
    show_version_info();
#endif

    const int read_only = 1;

    struct Device * const dev = mount_dev ( filename, read_only );
    if ( ! dev ) {
        adfEnvCleanUp();
        return NULL;
    }

    struct Volume * const vol = mount_volume ( dev, volume, read_only );
    if ( ! vol ) {
        adfUnMountDev ( dev );
        adfEnvCleanUp();
        return NULL;
    }

    adfimage_t * const adfimage = malloc ( sizeof ( adfimage_t ) );
    if ( ! adfimage ) {
        fprintf ( stderr, "Error: Cannot allocate memory for adfimage data\n" );
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
    printf ("\nfile size: %ld\n", adfimage->size );
#endif

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


adfimage_dentry_t adfimage_get_root_dentry ( adfimage_t * const adfimage )
{
    adfimage_dentry_t adf_dentry = {
        .type = ADFVOLUME_DENTRY_NONE
    };

    struct bRootBlock rootBlock;
    struct Volume * const vol = adfimage->vol;

    if ( adfReadRootBlock ( vol, vol->rootBlock, &rootBlock ) != RC_OK )
        return adf_dentry;

    struct Entry entry;
    if ( adfEntBlock2Entry ( ( struct bEntryBlock * ) &rootBlock, &entry ) != RC_OK )
        return adf_dentry;

    if ( entry.type != ST_ROOT )
        return adf_dentry;

    adf_dentry.type = ADFVOLUME_DENTRY_DIRECTORY;
    adf_dentry.adflib_entry = entry;

    return adf_dentry;
}


static struct Entry * adflib_list_find ( struct List * const dentries,
                                         const char * const  entry_name )
{
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

        if ( strcmp ( dentry->name, entry_name ) == 0 ) {
            return dentry;
        }
    }
    return NULL;
}


adfimage_dentry_t adfimage_getdentry ( adfimage_t * const adfimage,
                                       const char * const pathname )
{
    adfimage_dentry_t adf_dentry = {
        .type = ADFVOLUME_DENTRY_NONE
    };

    // change the directory first (if necessary)
    char * dirpath_buf = strdup ( pathname );
    char * dir_path = dirname ( dirpath_buf );
    char * cwd = NULL;
    if ( strlen ( dir_path ) > 0 ) {
        cwd = strdup ( adfimage->cwd );
        adfimage_chdir ( adfimage, dir_path );
    }
    free ( dirpath_buf );

    // get directory list entries (for current directory)
    struct Volume * const vol = adfimage->vol;
    struct List * const dentries = adfGetDirEnt ( vol, vol->curDirPtr );
    if ( ! dentries ) {
        fprintf ( stderr, "adfimage_getdentry(): Error getting dir entries,"
                  "filename %s\n", pathname );
        return adf_dentry;
    }

    char * filename_buf = strdup ( pathname );
    char * filename = basename ( filename_buf );

    struct Entry * const dentry = adflib_list_find ( dentries, filename );

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
            fprintf ( stderr,
                      "adfimage_getdentry() error: unsupported type: %d\n",
                      dentry->type );
            adf_dentry.type = ADFVOLUME_DENTRY_UNKNOWN;
        }
    }

    free ( filename_buf );
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
    struct Volume * const vol = adfimage->vol;

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


int adfimage_read ( adfimage_t * const adfimage,
                    const char *       path,
                    char *             buffer,
                    size_t             size,
                    off_t              offset )
{
    // if necessary (file path is not in the main dir) - enter the directory
    // with the file to read
    char * dirpath_buf = strdup ( path );
    char * dir_path = dirname ( dirpath_buf );
    char * cwd = NULL;
    if ( strlen ( dir_path ) > 0 ) {
        cwd = strdup ( adfimage->cwd );
        if ( ! adfimage_chdir ( adfimage, dir_path ) ) {
            //log_info ( fs_state->logfile, "adffs_read(): Cannot chdir to the directory %s.\n",
            //           dir_path );
            free ( cwd );
            free ( dirpath_buf );
            return -ENOENT;
        }
    }
    free ( dirpath_buf );

    // get the filename
    char * filename_buf = strdup ( path );
    char * filename = basename ( filename_buf );

    // open the file
    struct Volume * const vol = adfimage->vol;
    struct File * file = adfOpenFile ( vol, filename, "r" );
    free ( filename_buf );
    if ( ! file ) {
        //log_info ( fs_state->logfile,
        //           "Error opening file: %s\n", path );
        return -ENOENT;
    }

    // seek and read the file
    adfFileSeek ( file, offset );
    int32_t bytes_read = adfReadFile ( file, size, ( unsigned char * ) buffer );

    // ... and close it
    adfCloseFile ( file );

    // go back to the working directory (if necessary)
    if ( cwd ) {
        adfimage_chdir ( adfimage, cwd );
        free ( cwd );
    }

    return bytes_read;
}

// return value: 0 on success, -1 on error
int adfimage_readlink ( adfimage_t * const adfimage,
                        const char *       path,
                        char *             buffer,
                        size_t             len_max )
{
    int status = 0;

    // if necessary (file path is not in the main dir) - enter the directory
    // with the file to read
    char * dirpath_buf = strdup ( path );
    char * dir_path = dirname ( dirpath_buf );
    char * cwd = NULL;
    if ( strlen ( dir_path ) > 0 ) {
        cwd = strdup ( adfimage->cwd );
        if ( ! adfimage_chdir ( adfimage, dir_path ) ) {
            //log_info ( fs_state->logfile, "adffs_readlink(): Cannot chdir to the directory %s.\n",
            //           dir_path );
            free ( cwd );
            free ( dirpath_buf );
            return -1;
        }
    }
    free ( dirpath_buf );

    // get the filename
    char * filename_buf = strdup ( path );
    char * filename = basename ( filename_buf );

    // get block of the directory
    struct Volume * const vol = adfimage->vol;
    struct bEntryBlock parent;
    if ( adfReadEntryBlock( vol, vol->curDirPtr, &parent ) != RC_OK ) {
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
                                          filename,
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
    free ( filename_buf );
    if ( cwd ) {
        adfimage_chdir ( adfimage, cwd );
        free ( cwd );
    }

    //strncpy ( buffer, "secret.S", len_max );
    return status;
}


static void show_version_info()
{
    // afd version info only for now
    printf ( "libadf version: %s\n", adfGetVersionNumber() );
    printf ( "libadf date:    %s\n", adfGetVersionDate() );
}



static struct Device *
    mount_dev ( char * const adf_filename,
                const BOOL   read_only )
{
    // mount device (ie. image file)
#ifdef DEBUG_ADFIMAGE
    printf ("Mounting file: %s\n", adf_filename );
#endif
    struct Device * const dev = adfMountDev ( adf_filename, read_only );
    if ( dev == NULL ) {
        fprintf ( stderr, "Error opening ADF file: %s\n", adf_filename );
    }

#ifdef DEBUG_ADFIMAGE
    printf ( "\nMounted device info:\n" );
    //adfDeviceInfo ( dev );
    printf ( "  size:\t\t%d\n  volumes:\t%d\n", dev->size, dev->nVol );
#endif

    return dev;
}

struct Volume *
    mount_volume ( struct Device * const dev,
                   unsigned int          partition,
                   BOOL                  read_only )
{
    // mount volume (volume/partition number, for floppies always 0 (?))
#ifdef DEBUG_ADFIMAGE
    printf ("\nMounting volume (partition) %d\n", partition );
#endif
    struct Volume * const vol = adfMount ( dev, (int) partition, read_only );
    if ( vol == NULL ) {
        fprintf ( stderr,  "Error opening volume %d.\n", partition );
    }

#ifdef DEBUG_ADFIMAGE
    adfVolumeInfo ( vol );
#endif
    return vol;
}


static long getFileSize ( const char * const filename )
{
    FILE * const file = fopen ( filename, "r" );
    if ( ! file ) {
        fprintf ( stderr, "Cannot open the file: %s\n", filename );
        return -1;
    }

    if ( fseek ( file, 0L, SEEK_END ) != 0 ) {
        fprintf ( stderr, "Cannot fseek the file: %s\n", filename );
        fclose ( file );
        return -1;
    }

    long flen = ftell ( file );
    if ( flen < 0 ) {
        fprintf ( stderr, "Incorrect size (%ld) of the file: %s.\n", flen, filename );
        fclose ( file );
        return -1;
    }
    return flen;
}

static void append_dir ( adfimage_t * const adfimage,
                         const char * const dir )
{
    // update current working dir.
    if ( strcmp ( adfimage->cwd, "/" ) != 0 )
        strcat ( adfimage->cwd, "/" );
    strcat ( adfimage->cwd, dir );
}
