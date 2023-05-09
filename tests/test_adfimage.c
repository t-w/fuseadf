#include <check.h>
#include <stdlib.h>

#include "../src/adfimage.h"


START_TEST ( test_check_framework )
{
    ck_assert ( true );
}
END_TEST


START_TEST ( test_adfimage_open_fail )
{
    adfimage_t * adf = adfimage_open ( "nonexistent.adf", 0, true, stdout );
    ck_assert_ptr_null ( adf );
}
END_TEST


START_TEST ( test_adfimage_open_close )
{
    adfimage_t * adf = adfimage_open ( "testdata/ffdisk0049.adf", 0, false, stdout );
    ck_assert_ptr_nonnull ( adf );
    
    adfimage_close ( &adf );
    ck_assert_ptr_null ( adf );
}
END_TEST


START_TEST ( test_adfimage_properties )
{
    adfimage_t * adf = adfimage_open ( "testdata/ffdisk0049.adf", 0, false, stdout );

    ck_assert_int_eq ( adf->size, 901120 );
    ck_assert_ptr_nonnull ( adf->dev );
    ck_assert_ptr_nonnull ( adf->vol );

    adfimage_close ( &adf );
}
END_TEST


START_TEST ( test_adfimage_getdentry )
{
    adfimage_t * adf = adfimage_open ( "testdata/ffdisk0049.adf", 0, false, stdout );

    adfimage_dentry_t dentry =
        adfimage_getdentry ( adf, "non-exestent-file.tst" );
    ck_assert_int_eq ( dentry.type, ADFVOLUME_DENTRY_NONE );

    dentry = adfimage_getdentry ( adf, "README.list49" );
    ck_assert_int_eq ( dentry.type, ADFVOLUME_DENTRY_FILE );

    dentry = adfimage_getdentry ( adf, "Plot" );
    ck_assert_int_eq ( dentry.type, ADFVOLUME_DENTRY_DIRECTORY );

    adfimage_close ( &adf );
}
END_TEST


START_TEST ( test_adfimage_getdentry_links )
{
    adfimage_t * adf = adfimage_open ( "testdata/testffs.adf", 0, false, stdout );
    ck_assert_ptr_nonnull ( adf );

    adfimage_dentry_t dentry =
        adfimage_getdentry ( adf, "non-existent.tst" );
    ck_assert_int_eq ( dentry.type, ADFVOLUME_DENTRY_NONE );

    dentry = adfimage_getdentry ( adf, "dir_1" );
    ck_assert_int_eq ( dentry.type, ADFVOLUME_DENTRY_DIRECTORY );

    dentry = adfimage_getdentry ( adf, "secret.S" );
    //dentry = adfimage_getdentry ( adf, "emptyfile" );
    ck_assert_int_eq ( dentry.type, ADFVOLUME_DENTRY_FILE );

    dentry = adfimage_getdentry ( adf, "hlink_dir1" );
    ck_assert_int_eq ( dentry.type, ADFVOLUME_DENTRY_LINKDIR );

    dentry = adfimage_getdentry ( adf, "hlink_dir2" );
    ck_assert_int_eq ( dentry.type, ADFVOLUME_DENTRY_LINKDIR );

    dentry = adfimage_getdentry ( adf, "hlink_blue" );
    ck_assert_int_eq ( dentry.type, ADFVOLUME_DENTRY_LINKFILE );

    dentry = adfimage_getdentry ( adf, "slink_dir1" );
    ck_assert_int_eq ( dentry.type, ADFVOLUME_DENTRY_SOFTLINK );

    adfimage_close ( &adf );
}
END_TEST



START_TEST ( test_adfimage_getcwd )
{
    adfimage_t * adf = adfimage_open ( "testdata/ffdisk0049.adf", 0, false, stdout );

    const char * cwd = adfimage_getcwd ( adf );
    ck_assert_ptr_nonnull ( cwd );
    ck_assert_str_eq ( "/", cwd );

    adfimage_close ( &adf );
}
END_TEST



START_TEST ( test_adfimage_chdir )
{
    adfimage_t * adf = adfimage_open ( "testdata/ffdisk0049.adf", 0, false, stdout );

    BOOL result = adfimage_chdir ( adf, "non-exestent-dir" );
    ck_assert ( ! result );

    // go to top-level
    const char * testdir = "/";
    result = adfimage_chdir ( adf, testdir );
    ck_assert ( result );
    const char * cwd = adfimage_getcwd ( adf );
    ck_assert_str_eq ( testdir, cwd );

    result = adfimage_chdir ( adf, "Plot" );
    ck_assert ( result );
    cwd = adfimage_getcwd ( adf );
    ck_assert_str_eq ( "/Plot", cwd );

    adfimage_dentry_t dentry =
        adfimage_getdentry ( adf, "non-exestent-file.tst" );
    ck_assert_int_eq ( dentry.type, ADFVOLUME_DENTRY_NONE );

    dentry = adfimage_getdentry ( adf, "plot.c" );
    ck_assert_int_eq ( dentry.type, ADFVOLUME_DENTRY_FILE );

    // go to top-level
    testdir = "/";
    result = adfimage_chdir ( adf, testdir );
    ck_assert ( result );
    cwd = adfimage_getcwd ( adf );
    ck_assert_str_eq ( testdir, cwd );

    // test case-insensitive chdir
    result = adfimage_chdir ( adf, "plot" );
    ck_assert ( result );
    cwd = adfimage_getcwd ( adf );
    ck_assert_str_eq ( "/plot", cwd );

    dentry = adfimage_getdentry ( adf, "non-exestent-file.tst" );
    ck_assert_int_eq ( dentry.type, ADFVOLUME_DENTRY_NONE );

    dentry = adfimage_getdentry ( adf, "plot.c" );
    ck_assert_int_eq ( dentry.type, ADFVOLUME_DENTRY_FILE );

    // this fails - meaning getdentry is not case-insensitive(!)
    //dentry = adfimage_getdentry ( adf->vol, "ploT.c" );
    //ck_assert_int_eq ( dentry.type, ADFVOLUME_DENTRY_FILE );

    // test multi-level subdir.
    testdir = "/";
    result = adfimage_chdir ( adf, testdir );
    ck_assert ( result );
    cwd = adfimage_getcwd ( adf );
    ck_assert_str_eq ( testdir, cwd );

    result = adfimage_chdir ( adf, "Polygon" );
    ck_assert ( result );
    cwd = adfimage_getcwd ( adf );
    ck_assert_str_eq ( "/Polygon", cwd );

    dentry = adfimage_getdentry ( adf, "iffwriter" );
    ck_assert_int_eq ( dentry.type, ADFVOLUME_DENTRY_DIRECTORY );

    result = adfimage_chdir ( adf, "iffwriter" );
    ck_assert ( result );
    cwd = adfimage_getcwd ( adf );
    ck_assert_str_eq ( "/Polygon/iffwriter", cwd );

    dentry = adfimage_getdentry ( adf, "iffwriter.h" );
    ck_assert_int_eq ( dentry.type, ADFVOLUME_DENTRY_FILE );


    // one more test (of one failing at some point...)
    testdir = "/";
    result = adfimage_chdir ( adf, testdir );
    ck_assert ( result );
    cwd = adfimage_getcwd ( adf );
    ck_assert_str_eq ( testdir, cwd );

    result = adfimage_chdir ( adf, "Multidef" );
    ck_assert ( result );
    cwd = adfimage_getcwd ( adf );
    ck_assert_str_eq ( "/Multidef", cwd );


    adfimage_close ( &adf );
}
END_TEST


START_TEST ( test_adfimage_read )
{
    adfimage_t * adf = adfimage_open ( "testdata/ffdisk0049.adf", 0, false, stdout );

    char buf[1024];

    // read a file in the main directory
    const char * filename = "README.list49";
    adfimage_dentry_t dentry = adfimage_getdentry ( adf, filename );
    ck_assert_int_eq ( dentry.type, ADFVOLUME_DENTRY_FILE );
    int bytes_read = adfimage_read ( adf, filename, buf, 10, 0 );
    ck_assert_int_eq ( bytes_read, 10 );
    const char * cwd = adfimage_getcwd ( adf );
    ck_assert_str_eq ( "/", cwd );

    // reading a file with a directory in its pathname
    filename = "plot/plot.h";
    dentry = adfimage_getdentry ( adf, filename );
    ck_assert_int_eq ( dentry.type, ADFVOLUME_DENTRY_FILE );
    bytes_read = adfimage_read ( adf, filename, buf, 10, 0 );
    ck_assert_int_eq ( bytes_read, 10 );
    cwd = adfimage_getcwd ( adf );
    ck_assert_str_eq ( "/", cwd );

    adfimage_close ( &adf );
}
END_TEST


START_TEST ( test_adfimage_read_large_file )
{
    adfimage_t * adf = adfimage_open ( "testdata/ffdisk0049.adf", 0, false, stdout );

    adfimage_dentry_t dentry = adfimage_getdentry ( adf, "Polygon/polynums.c" );
    ck_assert_int_eq ( dentry.type, ADFVOLUME_DENTRY_FILE );

    char buf[1024 * 1024];

    // failing reading files larger that ~50k
    int bytes_read = adfimage_read ( adf, "Polygon/polynums.c", buf, 10, 0 );
    ck_assert_int_eq ( bytes_read, 10 );

    bytes_read = adfimage_read ( adf, "Polygon/polynums.c", buf, 10, 10 );
    ck_assert_int_eq ( bytes_read, 10 );

    bytes_read = adfimage_read ( adf, "Polygon/polynums.c", buf, 10, 0x100 );
    ck_assert_int_eq ( bytes_read, 10 );

    bytes_read = adfimage_read ( adf, "Polygon/polynums.c", buf, 10, 0x8000 );
    ck_assert_int_eq ( bytes_read, 10 );

    bytes_read = adfimage_read ( adf, "Polygon/polynums.c", buf, 59854, 0 );
    ck_assert_int_eq ( bytes_read, 59854 );

    bytes_read = adfimage_read ( adf, "Polygon/polynums.c", buf, 1000 * 10, 0x893f );
    ck_assert_int_eq ( bytes_read, 1000 * 10 );

    bytes_read = adfimage_read ( adf, "Trees/BCS", buf, 1000 * 10, 0x893f );
    ck_assert_int_eq ( bytes_read, 1000 * 10 );

    // read fails for offset 0x8940 (1 bytes can be read at 0x893f)
    bytes_read = adfimage_read ( adf, "Polygon/polynums.c", buf, 1, 0x8940 );
    ck_assert_int_eq ( bytes_read, 1 );

    bytes_read = adfimage_read ( adf, "Trees/BCS", buf, 1, 0x8940 );
    ck_assert_int_eq ( bytes_read, 1 );

    bytes_read = adfimage_read ( adf, "Polygon/polynums.c", buf, 1, 0x0bfe8 );
    ck_assert_int_eq ( bytes_read, 1 );

    bytes_read = adfimage_read ( adf, "Polygon/polynums.c", buf, 1, 0x0bffe );
    ck_assert_int_eq ( bytes_read, 1 );

    bytes_read = adfimage_read ( adf, "Polygon/polynums.c", buf, 1, 0x0bfff );
    ck_assert_int_eq ( bytes_read, 1 );

    bytes_read = adfimage_read ( adf, "Polygon/polynums.c", buf, 1, 0x0c000 );
    ck_assert_int_eq ( bytes_read, 1 );

    const char * cwd = adfimage_getcwd ( adf );
    ck_assert_str_eq ( "/", cwd );

    adfimage_close ( &adf );
}
END_TEST


START_TEST ( test_adfimage_read_hard_link_file )
{
    adfimage_t * adf = adfimage_open ( "testdata/testffs.adf", 0, false, stdout );
    ck_assert_ptr_nonnull ( adf );

    const char hlink_file[] = "hlink_blue";

    adfimage_dentry_t dentry = adfimage_getdentry ( adf, hlink_file );
    ck_assert_int_eq ( dentry.type, ADFVOLUME_DENTRY_LINKFILE );

    char buf [ 128 ];

    int bytes_read = adfimage_read ( adf, hlink_file, buf, 10, 0 );
    ck_assert_int_eq ( bytes_read, 10 );
    ck_assert_mem_eq ( buf, "GIF87a", 6 );

    adfimage_close ( &adf );
}
END_TEST


Suite * adfimage_suite ( void )
{
    Suite * s = suite_create ( "adfimage" );
    
    TCase * tc = tcase_create ( "check framework" );
    tcase_add_test ( tc, test_check_framework );
    suite_add_tcase ( s, tc );

    tc = tcase_create ( "adfimage open nonexistent - fail" );
    tcase_add_test ( tc, test_adfimage_open_fail );
    suite_add_tcase ( s, tc );

    tc = tcase_create ( "adfimage open close" );
    tcase_add_test ( tc, test_adfimage_open_close );
    suite_add_tcase ( s, tc );

    tc = tcase_create ( "adfimage properties" );
    tcase_add_test ( tc, test_adfimage_properties );
    suite_add_tcase ( s, tc );

    tc = tcase_create ( "adfimage getdentry" );
    tcase_add_test ( tc, test_adfimage_getdentry );
    suite_add_tcase ( s, tc );

    tc = tcase_create ( "adfimage getdentry_links" );
    tcase_add_test ( tc, test_adfimage_getdentry_links );
    suite_add_tcase ( s, tc );

    tc = tcase_create ( "adfimage getcwd" );
    tcase_add_test ( tc, test_adfimage_getcwd );
    suite_add_tcase ( s, tc );

    tc = tcase_create ( "adfimage chdir" );
    tcase_add_test ( tc, test_adfimage_chdir );
    suite_add_tcase ( s, tc );

    tc = tcase_create ( "adfimage read" );
    tcase_add_test ( tc, test_adfimage_read );
    suite_add_tcase ( s, tc );

    tc = tcase_create ( "adfimage read large file" );
    tcase_add_test ( tc, test_adfimage_read_large_file );
    suite_add_tcase ( s, tc );

    tc = tcase_create ( "adfimage read hard link file" );
    tcase_add_test ( tc, test_adfimage_read_hard_link_file );
    suite_add_tcase ( s, tc );

    return s;
}


int main ( void )
{
    Suite * s = adfimage_suite();
    SRunner * sr = srunner_create ( s );

    srunner_run_all ( sr, CK_VERBOSE ); //CK_NORMAL );
    int number_failed = srunner_ntests_failed ( sr );
    srunner_free ( sr );
    return ( number_failed == 0 ) ?
        EXIT_SUCCESS :
        EXIT_FAILURE;
}
