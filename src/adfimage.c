
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


adfvolume_dentry_t adfvolume_getdentry ( struct Volume * const vol,
                                         const char * const    name )
{
    adfvolume_dentry_t adf_dentry = { 
        .type = ADFVOLUME_DENTRY_NONE
    };

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


/*
static int cdTrackRead ( const cdImage_t * const cdimage,
                         const int               trackNumber,
                         char * const            buffer,
                         const size_t            size,
                         const off_t             offset )
{
    FILE * const fimg = fopen ( cdimage->filenameBin, "r" );
    if ( ! fimg )
        return -ENOENT; // to improve...

    const cdTrackInfo_t * const track    = & cdimage->tracks [ trackNumber ];
    fseek ( fimg, track->begin + offset, SEEK_SET );
    int bytesToRead = ( offset + size > track->dataLength ) ?
        track->dataLength - offset : size;
    size_t readBytes = fread ( buffer, 1, bytesToRead, fimg );
    fclose ( fimg );
    return readBytes;
}
*/

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

/*
// Allocates the buffer for the file contents and read the file data to ip
// Sets the buffer to point to the allocated data (NULL on error).
// Returns size of read data (a value < 0 on error ),
// 
static int
    readFile ( const char * const filename,
               char **            buffer )
{
    *buffer = NULL;

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

    int flen = ftell ( file );
    if ( flen < 0 ) {
        fprintf ( stderr, "Incorrect size (%d) of the file: %s.\n", flen, filename );
        fclose ( file );
        return -1;
    }
    fseek ( file, 0L, SEEK_SET );
    
    *buffer = ( char * ) malloc ( flen );
    int bytesRead = fread ( *buffer, 1, flen, file );
    fclose ( file );
    if ( bytesRead != flen ) {
        fprintf ( stderr, "Size of read data (%d) != file length (%d)"
                  "- error reading the file: %s.\n", bytesRead, flen, filename );
        free ( *buffer );
        *buffer = NULL;
        return -1;
    }    
    return bytesRead;
}

static const char * const trackInfoStr  = " TRACK ";
static const char * const trackIndexStr = " INDEX ";

static int getNumberOfTracks ( const char * cueFileData,
                               const int    dataSize )
{
    int ntracks = 0;
    while ( cueFileData = strstr ( cueFileData, trackInfoStr ) ) {
        ntracks++;
        cueFileData += sizeof ( trackInfoStr - 1 );  // without NULL
    }
    //printf ("Tracks: %d\n", ntracks );
    return ntracks;
}

static int getTrackModeFromCueString ( const char * const trackModeCueStr )
{
    for ( int mode = 0; mode <= CD_TRACK_MAX ; mode++ ) {
        //printf ("Checking '%s' vs '%s'\n", trackModeCueStr, CD_TRACK_CUE_MODE [ mode ] );
        if ( strcmp ( trackModeCueStr, CD_TRACK_TYPE_INFO [ mode ].modeNameCue ) == 0 )
             return mode;
    }
    return CD_TRACK_MODE_UNKNOWN;
}


static int timeToBlockOffset ( const char * const time,
                               int                mode )
{
    int minutes = atoi( &time[0] ),
        seconds = atoi( &time[3] ),
        frames  = atoi( &time[6] );
    // track's offset in blocks = offset given in CUE in time / frames
    // (1 frame -> 1 block of data)
    long first_block = 75 * ( minutes * 60 + seconds ) + frames;
    long offset = first_block * CD_SECTOR_SIZE;

//#if DEBUG_CDIMAGE == 2
    
    //fprintf ( stderr, " min: %d, sec: %d. frames: "
    //          "\n-> total frames (tracks' offset in blocks): %d"
    //          "\n   track's offset in bytes: %d\n",
    //          minutes, seconds, frames, first_block, offset );
    
    return offset;
}


static int getTracksInfo ( cdImage_t * const cdimage,
                           const char *      cueFileData,
                           const int         dataSize )
{
    cdTrackInfo_t * const tracks = cdimage->tracks;
    const char * const dataEnd = cueFileData + dataSize;
    for ( int i = 0 ;
          cueFileData < dataEnd &&
              ( cueFileData = strstr ( cueFileData, trackInfoStr ) ) ;
          i++ )
    {
        int num;
        const char buf[32];
        if ( sscanf ( cueFileData, " TRACK %d %s", &num, &buf ) != 2 ) {
            fprintf ( stderr, "Error parsing info for track %d\n", i );
            return i;
        }
        //printf ("trackNum: %d, mode: '%s'\n", num, buf );
        tracks [ i ].mode = getTrackModeFromCueString ( buf );
        //printf ( "mode: %d\n", tracks [ i ].mode );
        
        cueFileData = strstr ( cueFileData, trackIndexStr );
        if ( sscanf ( cueFileData, " INDEX %d %s", &num, &buf ) != 2 ) {
            fprintf ( stderr, "Error parsing index info for track %d\n", i );
            return i;
        }
        //printf ("indexNum: %d, time offset: %s\n", num, buf );

        tracks [ i ].begin = ( tracks [ i ].mode >= 0 &&
                               tracks [ i ].mode <= CD_TRACK_MAX ) ?
            timeToBlockOffset ( buf, tracks [ i ].mode ) : 0;
        
        //printf ( "offset: %d\n", tracks [ i ].begin );
    }

    // set tracks' ends ( without the last one )
    for ( int i = 0 ; i < cdimage->ntracks - 1; ++i )
        tracks [ i ].end = tracks [ i + 1 ].begin;

    // for the last track - the end offset is the end of the image
    tracks [ cdimage->ntracks - 1 ].end = cdimage->size;  

    // set track's length
    for ( int i = 0 ; i < cdimage->ntracks ; ++i ) {
        cdTrackInfo_t * const     track         = & tracks [ i ];
        const cdTrackTypeInfo_t * const trackTypeInfo = & CD_TRACK_TYPE_INFO [ track->mode ];
        const int trackSizeInSectors = ( track->end - tracks->begin ) / CD_SECTOR_SIZE;
        track->dataLength = trackSizeInSectors * trackTypeInfo->dataSize;
        //printf ( "track %d: ", i + 1 );
        //printf ( " block size raw %d, data offset in block %d, data size in block %d"
        //         ", track size in blocks %d", blockSizeRaw, dataOffsetInBlock,
        //         dataSizeInBlock, trackSizeInBlocks );
        //printf ( ", data len: %d\n", track->dataLength  );
    }
    return cdimage->ntracks;
}


static int showTracksInfo ( const cdImage_t * const cdimage )
{
    const cdTrackInfo_t * const tracks = cdimage->tracks;
    for ( int i = 0 ; i < cdimage->ntracks; i++ ) {
        printf ( "track%02d:  mode %-16s begin %9ld,  end %9ld,  length %9ld\n", i+1,
                 CD_TRACK_TYPE_INFO [ tracks [ i ].mode ].modeName,
                 tracks [ i ].begin,
                 tracks [ i ].end,
                 tracks [ i ].dataLength );
    }
}
*/
