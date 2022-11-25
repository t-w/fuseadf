
#include "log.h"

#include <errno.h>
#include <stdarg.h>
#include <string.h>


FILE * log_open ( const char * const log_file_path )
{
    FILE *flog = fopen ( log_file_path, "w" );
    if ( flog == NULL ) {
	fprintf ( stderr, "Error opening log file %s: %s",
                  log_file_path, strerror ( errno ) );
        return NULL;
    }
    setlinebuf ( flog );
    return flog;
}


void log_info ( FILE * const flog,
                const char * const format, ... )
{
    va_list ap;
    va_start ( ap, format );
    if ( flog )
        vfprintf ( flog, format, ap );
}


void log_error ( FILE * const flog,
                 const char * const error_context )
{
    log_info ( flog, "ERROR %s: %s\n", error_context, strerror ( errno ) );
}
