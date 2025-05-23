
#ifndef ADFIMAGE_H
#define ADFIMAGE_H

#include <adflib.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// check the max. value for Amiga filesystems(!)
#define ADFIMAGE_MAX_PATH 1024

typedef struct adfimage {
    const char * filename;

    struct AdfDevice * dev;
    struct AdfVolume * vol;

    struct stat fstat;

    char cwd [ ADFIMAGE_MAX_PATH ];

//    FILE * logfile;
} adfimage_t;


adfimage_t * adfimage_open ( char * const filename,
                             unsigned int volume,
                             bool         read_only,
                             const bool   ignore_checksum_errors );

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

// permission translation limited to rwx (read/write/execute)
// on Linux (*nix size) all the same for user/group/others,
// except for write (only user)
enum ADF_PERMISSIONS {
    ADF_PERM_READ    = 1,
    ADF_PERM_WRITE   = 2,
    ADF_PERM_EXECUTE = 4
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

static inline bool adfimage_dentry_valid( const adfimage_dentry_t * const dentry ) {
    return ( dentry->type > ADFVOLUME_DENTRY_NONE &&
             dentry->type < ADFVOLUME_DENTRY_UNKNOWN );
}

int adfimage_getperm( adfimage_dentry_t * const dentry );

bool adfimage_setperm( adfimage_t * const adfimage,
                       const char *       path,
                       const int          perms );

const char * adfimage_getcwd ( const adfimage_t * const adfimage );

bool adfimage_chdir ( adfimage_t * const adfimage,
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
