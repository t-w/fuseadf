
#ifndef LOG_H
#define LOG_H

#include <stdio.h>

//typedef struct {
//    FILE * logfile;
//    int level;
//} log_t;

FILE * log_open ( const char * const log_file_path );
void log_close ( FILE * const       flog );

void vlog_info ( FILE * const       flog,
                 const char * const format,
                 va_list            ap );

void log_info ( FILE * const       flog,
                const char * const format, ... );

void log_error ( FILE * const       flog,
                 const char * const error_context );
#endif
