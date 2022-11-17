
#ifndef __ADFIMAGE_H__
#define __ADFIMAGE_H__

#include <adflib.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>


typedef struct adfimage {
    const char * filename;
    long size;

    struct Device * dev;
    struct Volume * vol;

    //int ntracks;
    //cdTrackInfo_t * tracks;
    struct stat fstat;
} adfimage_t;


adfimage_t * adfimage_open ( char * const filename );

//void adfimage_close ( adfimage_t * const adfimage );
void adfimage_close ( adfimage_t ** adfimage );

enum {
    ADFVOLUME_DENTRY_NONE,
    ADFVOLUME_DENTRY_FILE,
    ADFVOLUME_DENTRY_DIRECTORY
};

typedef struct adfvolume_dentry {
    // name ?
    int type;
    int size;
    // ...
} adfvolume_dentry_t;

adfvolume_dentry_t adfvolume_getdentry ( struct Volume * const vol,
                                         const char * const    name );

BOOL adfvolume_chdir ( struct Volume * const vol,
                       const char *          path );

int adfimage_read ( adfimage_t * const adfimage,
                    const char *       path,
                    char *             buffer,
                    size_t             size,
                    off_t              offset );

#endif
