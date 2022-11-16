
#include "config.h"
#include "adffs.h"

#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


void usage();

void parse_args (  const int argc,
                   char **   argv,
                   char **   adf_filename,
                   char **   mount_point );

void drop_nonfuse_args ( int *   argc,
                         char ** argv );


int main ( int    argc,
           char * argv[] )
{
    // forbid running as root (a prototype program interacting with kernel space...)
    if ( ( getuid() == 0 ) ||
         ( geteuid() == 0 ) )
    {
    	fprintf ( stderr, "fuseadf is a prototype - do not use as root.\n");
        exit ( EXIT_FAILURE );
    }
    char * adf_filename = NULL;
    char * mount_point  = NULL;
    parse_args ( argc, argv, &adf_filename, &mount_point );
    if ( ! adf_filename || ! mount_point )
        exit ( EXIT_FAILURE );

    struct adffs_state adffs_data;

    adffs_data.adfimage = adfimage_open ( adf_filename );
    if ( adffs_data.adfimage == NULL ) {
	fprintf ( stderr, "Cannot open cdimage %s.", adf_filename );
        exit ( EXIT_FAILURE );
    }
    adffs_data.mountpoint = mount_point;

    drop_nonfuse_args ( &argc, argv );

#ifdef DEBUG_ADFFS
    const char * const log_filename = "fuseadf.log";
    adffs_data.logfile = log_open ( log_filename );
    if ( ! adffs_data.logfile ) {
        fprintf ( stderr, "Cannot open log file: %s\n", log_filename );
        exit ( EXIT_FAILURE );
    }
#endif

#ifdef DEBUG_ADFFS
    fprintf ( stderr, "-> fuse_main()\n" );
#endif

    int fuse_status = fuse_main ( argc, (char **) argv, &adffs_oper, &adffs_data );

#ifdef DEBUG_ADFFS
    fprintf ( stderr, "fuse_main -> %d\n", fuse_status );
#endif

    return fuse_status;
}



void usage()
{
    fprintf ( stderr, "usage:\tfuse [fuse/mount options] [-p partiion] diskimage_adf mount_point\n"
              "\n\t-p partition - partition/volume number, default: 0\n"
              "\nFUSE options (for details see FUSE documentation):\n" 
              "\t-f\t\tforeground (do not daemonize)\n"
              "\t-d\t\tforeground with debug info\n"
              "\t-s\t\tsingle-threaded\n" );
}


void parse_args (  const int argc,
                   char **   argv,
                   char **   adf_filename,
                   char **   mount_point )
{
    *adf_filename = NULL;
    *mount_point  = NULL;

    const char * valid_options = "pdshv";
    
    for ( int opt = getopt ( argc, ( char * const * ) argv, valid_options ) ;
          opt != -1 ;
          opt = getopt ( argc, ( char * const * ) argv, valid_options ) )
    {
        switch ( opt ) {
        case 'p':
            // set partition number ...
            continue;
            // fuse options ( /usr/include/fuse/fuse_common.h )
        case 'd':
        case 's':
            continue;
        case 'h':   // check what it is for in FUSE (for now use as "help")
            usage();
            return;
        case 'v':
            printf ( PACKAGE_STRING "  ( fuse build version %d.%d, runtime library version %d )\n",
                     FUSE_MAJOR_VERSION, FUSE_MINOR_VERSION, fuse_version() );
        default:
            return;            
        }
    }
    if ( optind != argc - 2 )
        return;
    *adf_filename = argv [ argc - 2 ];
    *mount_point = realpath ( argv [ argc - 1 ], NULL );
}


void drop_nonfuse_args ( int *   argc,
                         char ** argv )
{
    // only one (cdimage_cue) for now
    argv [ *argc - 2 ] = argv [ *argc - 1 ];
    argv [ *argc - 1 ] = NULL;
    --( *argc );
}
