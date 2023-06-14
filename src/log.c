
#include "log.h"

#include <errno.h>
#include <stdarg.h>
#include <string.h>


FILE * log_open ( const char * const log_file_path )
{
    FILE * const flog = fopen ( log_file_path, "w" );
    if ( flog == NULL ) {
	fprintf ( stderr, "Error opening log file %s: %s",
                  log_file_path, strerror ( errno ) );
        return NULL;
    }
    setlinebuf ( flog );
    return flog;
}


void log_close ( FILE * const flog )
{
    fclose ( flog );
}


void vlog_info ( FILE * const       flog,
                 const char * const format,
                 va_list            ap )
{
    if ( flog ) {
        vfprintf ( flog, format, ap );
        fprintf ( flog, "\n" );
    }
}


void log_info ( FILE * const       flog,
                const char * const format, ... )
{
    va_list ap;
    vlog_info ( flog, format, ap );
}


void log_error ( FILE * const       flog,
                 const char * const error_context )
{
    log_info ( flog, "ERROR %s: %s\n", error_context, strerror ( errno ) );
}
