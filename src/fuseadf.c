
#include "config.h"
#include "adffs.h"

#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct cmdline_options_s {
    char *       adf_filename;
    char *       mount_point;
    unsigned int adf_volume;
    bool         help,
                 version;
} cmdline_options_t;

void usage();

bool parse_args (  int *               argc,
                   char **             argv,
                   cmdline_options_t * options );

void drop_arg ( int * argc, char ** argv, int index );
void drop_args ( int * argc, char ** argv, int index, int num );
void show_argv ( int argc, char ** argv );

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

    cmdline_options_t options;
    //show_argv ( argc, argv );
    if ( ! parse_args ( &argc, argv, &options ) ) {
        //show_argv ( argc, argv );
        if ( ! options.adf_filename || ! options.mount_point )
            fprintf ( stderr, "Usage info:  fuseadf -h\n" );
        exit ( EXIT_FAILURE );
    }
    //show_argv ( argc, argv );
    if ( options.help ) {
        usage();
        exit ( EXIT_SUCCESS );
    }

    if ( options.version ) {
        printf ( PACKAGE_STRING
                 "  ( fuse build version %d.%d, runtime library version %d )\n",
                 FUSE_MAJOR_VERSION, FUSE_MINOR_VERSION, fuse_version() );
        exit ( EXIT_SUCCESS );
    }

    struct adffs_state adffs_data;

    adffs_data.adfimage = adfimage_open ( options.adf_filename,
                                          options.adf_volume );
    if ( adffs_data.adfimage == NULL ) {
	fprintf ( stderr, "Cannot mount adf image: %s, "
                  "volume/partition: %d - aborting...\n",
                  options.adf_filename, options.adf_volume );
        exit ( EXIT_FAILURE );
    }
    adffs_data.mountpoint = options.mount_point;

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
              "\n\t-p partition - partition/volume number (0-10), default: 0\n"
              "\nFUSE options (for details see FUSE documentation):\n" 
              "\t-f\t\tforeground (do not daemonize)\n"
              "\t-d\t\tforeground with debug info\n"
              "\t-s\t\tsingle-threaded\n" );
}


bool parse_args (  int *               argc,
                   char **             argv,
                   cmdline_options_t * options )
{
    options->adf_filename = NULL;
    options->mount_point  = NULL;
    options->adf_volume   = 0;
    options->help         = false;
    options->version      = false;

    const char * valid_options = "p:dshv";
    int opt;
    while ( ( opt = getopt ( *argc, argv, valid_options ) ) != -1 ) {
        switch ( opt ) {
        case 'p': {
            // set partition number ...
            char * endptr = NULL;
            options->adf_volume = ( unsigned int ) strtoul ( optarg, &endptr, 10 );
            if ( endptr == optarg ) {
                fprintf ( stderr, "Incorrect partition/volume number.\n" );
                return false;
            }

            // is there a max. number of partitions for Amiga???
            if ( options->adf_volume > 10 ) {
                fprintf ( stderr, "Incorrect partition/volume number %u\n",
                          options->adf_volume );
                return false;
            }
            optind -= 2;
            drop_args ( argc, argv, optind, 2 );
            continue;
        }
        // fuse options ( /usr/include/fuse/fuse_common.h )
        case 'd':
        case 'f':
        case 's':
            continue;

        case 'h':   // check what it is for in FUSE (for now use as "help")
            options->help = true;
            optind--;
            drop_arg ( argc, argv, optind );
            return true;

        case 'v':
            options->version = true;
            optind--;
            drop_arg ( argc, argv, optind );
            return true;

        default:
            return false;
        }
    }

    if ( optind != *argc - 2 )
        return false;

    options->adf_filename = argv [ *argc - 2 ];
    options->mount_point  = realpath ( argv [ *argc - 1 ], NULL );

    drop_arg ( argc, argv, *argc - 2);

    return true;
}

// delete an argument from argv at index
void drop_arg ( int * argc, char ** argv, int index )
{
    for ( int i = index ; i < *argc - 1 ; i++ )
         argv [ i ] = argv [ i + 1 ];

    (*argc)--;
    argv [ *argc ] = NULL;
}

// delete num of arguments from argv at index
void drop_args ( int * argc, char ** argv, int index, int num )
{
    for ( int i = index ; i < *argc - num ; i++ )
        argv [ i ] = argv [ i + num ];

    for ( int i = *argc - num ; i < *argc ; i++ )
        argv [ i ] = NULL;

    (*argc) -= num;
}

/*
void show_argv ( int argc, char ** argv )
{
    printf ("argc: %d\n", argc );
    for ( int i = 1 ; i < argc ; ++i )
        printf ("option %d: %s\n", i, argv[i] );
}
*/
