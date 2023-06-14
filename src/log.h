
#ifndef __LOG_H__
#define __LOG_H__

#include <stdio.h>

//typedef struct {
//    FILE * logfile;
//    int level;
//} log_t;

FILE * log_open ( const char * const log_file_path );
void log_close ( void );
void log_info ( const char * const format, ... );
void log_error ( const char * const error_context );
#endif
