
#ifndef __ADFIMAGE_H__
#define __ADFIMAGE_H__

#include <adflib.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// check the max. value for Amiga filesystems(!)
#define ADFIMAGE_MAX_PATH 1024

typedef struct adfimage {
    const char * filename;
    long size;

    struct Device * dev;
    struct Volume * vol;

    //int ntracks;
    //cdTrackInfo_t * tracks;
    struct stat fstat;

    char current_directory [ ADFIMAGE_MAX_PATH ];
} adfimage_t;


adfimage_t * adfimage_open ( char * const filename );

//void adfimage_close ( adfimage_t * const adfimage );
void adfimage_close ( adfimage_t ** adfimage );

enum {
    ADFVOLUME_DENTRY_NONE,
    ADFVOLUME_DENTRY_FILE,
    ADFVOLUME_DENTRY_DIRECTORY
};

typedef struct adfimage_dentry {
    // name ?
    int type;
    int size;
    // ...
} adfimage_dentry_t;

adfimage_dentry_t adfimage_getdentry ( adfimage_t * const adfimage,
                                       const char * const name );

const char * adfimage_getcwd ( const adfimage_t * const adfimage );

BOOL adfimage_chdir ( adfimage_t * const adfimage,
                      const char *       path );

int adfimage_read ( adfimage_t * const adfimage,
                    const char *       path,
                    char *             buffer,
                    size_t             size,
                    off_t              offset );

#endif
