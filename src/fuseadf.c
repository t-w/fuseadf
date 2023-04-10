
#include "config.h"
#include "adffs.h"

#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct cmdline_options_s {
    char *       adf_filename;
    char *       mount_point;
    unsigned int adf_volume;
    bool         write_mode;
    char *       logging_file;
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

    printf ( "Opening image: %s, volume: %d. mode: %s\n",
             options.adf_filename,
             options.adf_volume,
             ( options.write_mode ) ? "read-write" : "read-only" );

    adffs_data.adfimage = adfimage_open ( options.adf_filename,
                                          options.adf_volume,
                                          ! options.write_mode );
    if ( adffs_data.adfimage == NULL ) {
	fprintf ( stderr, "Cannot mount adf image: %s, "
                  "volume/partition: %d - aborting...\n",
                  options.adf_filename, options.adf_volume );
        exit ( EXIT_FAILURE );
    }
    adffs_data.mountpoint = options.mount_point;

    if ( options.write_mode == true &&
         adffs_data.adfimage->dev->readOnly == TRUE )
    {
        printf ("Note: image opened in read-only mode.\n");
    }

    if ( options.logging_file ) {
        adffs_data.logfile = log_open ( options.logging_file );
        if ( ! adffs_data.logfile ) {
            fprintf ( stderr, "Cannot open log file: %s\n", options.logging_file );
            exit ( EXIT_FAILURE );
        }
        printf ( "fuseadf logging file: %s\n", options.logging_file );
    }

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
    fprintf ( stderr, "usage:\tfuseadf [fuse/mount options] [-p partition] [-l logging_file] diskimage_adf mount_point\n"
              "\n\t-p partition - partition/volume number (0-10), default: 0\n"
              "\t-l logfile   - enable logging and (optionally) specify logging file\n"
              "\t               (default: fuseadf.log)\n"
              "\nFUSE options (for details see FUSE documentation):\n" 
              "\t-o mount_option\tie. 'ro' for read-only mount (see: man fusermount)\n"
              "\t-f\t\tforeground (do not daemonize)\n"
              "\t-d\t\tforeground with debug info\n"
              "\t-s\t\tsingle-threaded\n" );
}


bool parse_args (  int *               argc,
                   char **             argv,
                   cmdline_options_t * options )
{
    // set default options
    memset ( options, 0, sizeof ( cmdline_options_t ) );
    options->write_mode = true;
    
    //const char * valid_options = "p:l::o:dshvwquzV";
    const char * valid_options = "p:l::o:dshvwV";
    int opt;
    while ( ( opt = getopt ( *argc, argv, valid_options ) ) != -1 ) {
        //printf ( "optind %d, opt %c, optarg %s\n", optind, ( char ) opt, optarg );
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

        case 'l': {
            if ( optarg ) {
                options->logging_file = optarg;
                optind -= 2;
                drop_args ( argc, argv, optind, 2 );
            } else {
                options->logging_file = "fuseadf.log";
                optind--;
                drop_arg ( argc, argv, optind );
            }
            continue;
        }

        // fuse options ( /usr/include/fuse/fuse_common.h )
        case 'd':
        case 'f':
        case 's':
            //    https://github.com/libfuse/libfuse/blob/master/doc/fusermount3.1
            //case 'q':
            //case 'u':
            //case 'z':
        case 'V':
            continue;
        case 'o':
            if ( strcmp ( optarg, "ro" ) == 0 )
                options->write_mode = false;
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
