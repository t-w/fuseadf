
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

    struct AdfDevice * dev;
    struct AdfVolume * vol;

    struct stat fstat;

    char cwd [ ADFIMAGE_MAX_PATH ];

//    FILE * logfile;
} adfimage_t;


adfimage_t * adfimage_open ( char * const filename,
                             unsigned int volume,
                             bool         read_only );

//void adfimage_close ( adfimage_t * const adfimage );
void adfimage_close ( adfimage_t ** adfimage );

enum {
    ADFVOLUME_DENTRY_NONE,
    ADFVOLUME_DENTRY_FILE,
    ADFVOLUME_DENTRY_DIRECTORY,
    ADFVOLUME_DENTRY_LINKFILE,
    ADFVOLUME_DENTRY_LINKDIR,
    ADFVOLUME_DENTRY_SOFTLINK,
    ADFVOLUME_DENTRY_UNKNOWN
};

typedef struct adfimage_dentry {
    // name ?
    int type;
    //int size;
    struct AdfEntry adflib_entry;   // entry as returned by ADFlib
    // ...
} adfimage_dentry_t;

int adfimage_count_cwd_entries ( adfimage_t * const adfimage );

int adfimage_count_dir_entries ( adfimage_t * const adfimage,
                                 const char * const dirname );

adfimage_dentry_t adfimage_get_root_dentry ( adfimage_t * const adfimage );

adfimage_dentry_t adfimage_getdentry ( adfimage_t * const adfimage,
                                       const char * const name );

const char * adfimage_getcwd ( const adfimage_t * const adfimage );

BOOL adfimage_chdir ( adfimage_t * const adfimage,
                      const char *       path );


struct AdfFile * adfimage_file_open ( adfimage_t * const adfimage,
                                      const char *       path,
                                      const AdfFileMode  mode );

void adfimage_file_close ( struct AdfFile * file );



int adfimage_read ( adfimage_t * const adfimage,
                    const char *       path,
                    char *             buffer,
                    size_t             size,
                    off_t              offset );

int adfimage_write ( adfimage_t * const adfimage,
                     const char *       path,
                     char *             buffer,
                     size_t             size,
                     off_t              offset );

int adfimage_readlink ( adfimage_t * const adfimage,
                        const char *       path,
                        char *             buffer,
                        size_t             len_max );

int adfimage_mkdir ( adfimage_t * const adfimage,
                     const char *       dirpath,
                     mode_t             mode );

int adfimage_rmdir ( adfimage_t * const adfimage,
                     const char *       rmdirpath );

int adfimage_create ( adfimage_t * const adfimage,
                      const char *       newfilepath,
                      mode_t             mode );


int adfimage_unlink ( adfimage_t * const adfimage,
                      const char *       unlinkpath );

int adfimage_file_truncate ( adfimage_t * const adfimage,
                             const char *       path,
                             const size_t       new_size );

int adfimage_file_rename ( adfimage_t * const adfimage,
                           const char * const src_pathstr,
                           const char * const dst_pathstr );

char * get_adflib_build_version ( void );
char * get_adflib_build_date ( void );
char * get_adflib_runtime_version ( void );
char * get_adflib_runtime_date ( void );


#endif
