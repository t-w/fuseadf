
#include "adfimage.h"

#include "log.h"

#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

//#define DEBUG_CDIMAGE 1

static void show_version_info();

static struct Device *
    mount_dev ( char * const adf_filename,
                const BOOL   read_only );

static struct Volume *
    mount_volume ( struct Device * const dev,
                   int                   partition,
                   BOOL                  read_only );

static long getFileSize ( const char * const filename );

static void append_dir ( adfimage_t * const adfimage,
                         const char * const dir );

adfimage_t *
    adfimage_open ( char * const filename )
{

    adfEnvInitDefault();

    // debug
    show_version_info();

    const int read_only = 1;

    struct Device * const dev = mount_dev ( filename, read_only );
    if ( ! dev ) {
        adfEnvCleanUp();
        return NULL;
    }

    struct Volume * const vol = mount_volume ( dev, 0, read_only );
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

    //cdimage->size = getFileSize ( cdimage->filenameBin );
    
    printf ("\nfile size: %ld\n", adfimage->size );


//#ifdef DEBUG_CDIMAGE 
//    showTracksInfo ( cdimage );
//#endif

    //free ( cueData );
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


adfimage_dentry_t adfimage_getdentry ( adfimage_t * const adfimage,
                                       const char * const name )
{
    adfimage_dentry_t adf_dentry = {
        .type = ADFVOLUME_DENTRY_NONE
    };

    struct Volume * const vol = adfimage->vol;
    struct List * const dentries = adfGetDirEnt ( vol, vol->curDirPtr );
    if ( ! dentries ) {
        fprintf ( stderr, "adfimage_getdentry(): Error getting dir entries,"
                  "filename %s\n", name );
        return adf_dentry;
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

        if ( strcmp ( dentry->name, name ) == 0 ) {
            // ....
            if ( dentry->type == ST_FILE ||
                 dentry->type == ST_LFILE
                 //dentry->type == ST_SOFT   // ?? softlink ?
                )
            {
                adf_dentry.type = ADFVOLUME_DENTRY_FILE;
            }
            else if ( dentry->type == ST_ROOT ||
                      dentry->type == ST_DIR ||
                      dentry->type == ST_LDIR
                      //dentry->type == ST_SOFT   // ?? softlink ?
                    )
            {
                adf_dentry.type = ADFVOLUME_DENTRY_DIRECTORY;
            } else {
                fprintf ( stderr, "adfimage_getdentry() error: unsupported type: %d\n",
                          dentry->type );
            }

            break;
        }
    }

    freeList ( dentries );

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
    //while ( *path == '/' ) // skip all leading '/' from the path
    //    path++;            // (normally, fuse always starts with a single '/')

    // if necessary (file path is not in the main dir) - enter the directory
    // with the file to read

    char * dirpath_buf = strdup ( path );
    char * dir_path = dirname ( dirpath_buf );
    if ( ! adfimage_chdir ( adfimage, dir_path ) ) {
        //log_info ( fs_state->logfile, "adffs_read(): Cannot chdir to the directory %s.\n",
        //           dir_path );
    }
    free ( dirpath_buf );

    char * filename_buf = strdup ( path );
    char * filename = basename ( filename_buf );
    struct Volume * const vol = adfimage->vol;
    struct File * file = adfOpenFile ( vol, filename, "r" );
    free ( filename_buf );
    if ( ! file ) {
        //log_info ( fs_state->logfile,
        //           "Error opening file: %s\n", path );
        return -ENOENT;
    }

    //void adfFileSeek(struct File *file, uint32_t pos);
    adfFileSeek ( file, offset );

    //int32_t adfReadFile(struct File* file, int32_t n, unsigned char *buffer);
    int32_t bytes_read = adfReadFile ( file, size, ( unsigned char * ) buffer );

    adfCloseFile ( file );
    adfToRootDir ( vol );

    return bytes_read;
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
    printf ("Mounting file: %s\n", adf_filename );
    //struct Device* adfMountDev( char* filename,BOOL ro);
    //struct Device * const dev = adfMountDev ( "d1.adf", readOnly );
    struct Device * const dev = adfMountDev ( adf_filename, read_only );
    if ( dev == NULL ) {
        fprintf ( stderr, "Error opening ADF file: %s\n", adf_filename );
    }

    printf ( "\nMounted device info:\n" );
    //adfDeviceInfo ( dev );
    printf ( "  size:\t\t%d\n  volumes:\t%d\n", dev->size, dev->nVol );
    return dev;
}

struct Volume *
    mount_volume ( struct Device * const dev,
                   int                   partition,
                   BOOL                  read_only )
{
    // mount volume (volume/partition number, for floppies always 0 (?))
    printf ("\nMounting volume (partition) %d\n", partition );
    //struct Device* adfMountDev( char* filename,BOOL ro);
    struct Volume * const vol = adfMount ( dev, partition, read_only );
    if ( vol == NULL ) {
        fprintf ( stderr,  "Error opening volume %d.\n", partition );
    }
    adfVolumeInfo ( vol );
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
