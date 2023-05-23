
#ifndef __FUSEADF_UTIL_H__
#define __FUSEADF_UTIL_H__

#include <assert.h>

static inline void pathstr_remove_trailing_slashes ( char * const path )
{
    char * slashptr;
    while ( ( slashptr = rindex ( path, '/' ) ) != NULL &&
            ( *( slashptr + 1 ) == '\0' ) )
    {
        *slashptr = '\0';
    }
}

static inline void pathstr_remove_double_slashes ( char * path )
{
    char * doubleslash_ptr;
    while ( ( doubleslash_ptr = strstr ( path, "//" ) ) != NULL ) {
        //for ( char * ch = doubleslash_ptr + 1;  *( ch + 1 ) != '\0';  ++ch )
        //    *ch = *( ch + 1 );
        char * ch = doubleslash_ptr + 1;
        do {
            *ch = *( ch + 1 );
        } while ( *( ch + 1 ) != '\0' );
        path = doubleslash_ptr;
    }
}

static inline const char * pathstr_get_relative ( const char * path )
{
    assert ( path != NULL );
    /* skip all leading '/' from the path
       (normally, fuse always starts paths with a single '/') */
    while ( *path == '/' )
        path++;
    return path;
}

/* convert unix/dos/... style parent '..' to AmigaDos '/', eg/
 *  Unix/DOS/etc:   cd ../tmp 
 *  AmigaDOS:       cd //tmp     (!!!)
 */
static inline void pathstr_dirparent_u2a ( char * const pathstr )
{
    char * parentptr = NULL;

    // replace '../' with '//'
    while ( ( parentptr = strstr ( pathstr, "../" ) ) != NULL ) {
        *parentptr = '/';
        parentptr++;
        do {
            *( parentptr ) = *( parentptr + 1 );
        } while ( *( parentptr + 1 ) != '\0' );
    }

    // replace '/..\0' with '//\0' (only at the end)
    while ( ( parentptr = strstr ( pathstr, "/.." ) ) != NULL ) {
        if ( *( parentptr + 3 ) == '\0' ) {
            parentptr++;
            *parentptr = '/';
            parentptr++;
            do {
                *( parentptr ) = *( parentptr + 1 );
            } while ( *( parentptr + 1 ) != '\0' );
        }
    }
}

static inline void pathstr_fuse2amigados ( char * const pathstr )
{
    pathstr_remove_double_slashes ( pathstr );
    //pathstr_remove_trailing_slashes ( pathstr );
    pathstr_dirparent_u2a ( pathstr );
}

static inline bool pathstr_is_non_empty ( const char * const path )
{
    assert ( path != NULL );
    // fuse should never request an action on an empty path
    if ( *path == '\0' )
        return false;
    return true;
}


typedef struct path_s {
    char * dirpath_buf,
         * dirpath,
         * entryname_buf,
         * entryname;
} path_t;


static inline path_t * path_create ( const char * const path_str )
{
    assert ( path_str != NULL );
    //const char * const path_relative = pathstr_get_relative ( path_str );

    path_t * const path = malloc ( sizeof ( path_t ) );
    if ( path == NULL )
        return NULL;

    path->dirpath_buf   = strdup ( path_str );
    if ( path->dirpath_buf == NULL ) {
        free ( path );
        return NULL;
    }
    path->dirpath       = dirname ( path->dirpath_buf );

    path->entryname_buf = strdup ( path_str );
    if ( path->entryname_buf == NULL ) {
        free ( path->dirpath_buf );
        free ( path );
        return NULL;
    }
    path->entryname     = basename ( path->entryname_buf );

    return path;
}

static inline void path_free ( path_t ** const path )
{
    free ( (*path)->dirpath_buf );
    free ( (*path)->entryname_buf );
    free ( *path );
    *path = NULL;
}

static inline bool path_is_dirpath_non_empty ( const path_t * const path )
{
    assert ( path != NULL );
    return ( *(path->dirpath) != '\0' );
}


static inline bool path_is_entryname_non_empty ( const path_t * const path )
{
    assert ( path != NULL );
    return ( *(path->dirpath) != '\0' );
}


#endif
