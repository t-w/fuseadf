
/* Utility functions created for debugging, not used at the moment */



static uint32_t checksum ( unsigned char * dataptr,
                           size_t          size )
{
    unsigned char chksum = 0;
    while ( size-- != 0 )
        //chksum -= *dataptr++;
        //chksum += *dataptr++;
        chksum ^= *dataptr++;
    return chksum;
}


static long getFileSize ( const char * const filename )
{
    FILE * const file = fopen ( filename, "r" );
    if ( ! file ) {
        fprintf ( stderr, "Cannot open the file: %s\n", filename );
        return -1;
    }

    if ( fseek ( file, 0L, SEEK_END ) != 0 ) {
        fprintf ( stderr, "Cannot fseek the file: %s\n", filename );
        fclose ( file );
        return -1;
    }

    long flen = ftell ( file );
    if ( flen < 0 ) {
        fprintf ( stderr, "Incorrect size (%ld) of the file: %s.\n", flen, filename );
        fclose ( file );
        return -1;
    }
    return flen;
}
