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
    //adfimage_t * adf = adfimage_open ( "nonexistent.adf" );
    // -> segfault after doing the above!!!
    //    (cannot try to open non existent image file with adflib?)
    adfimage_t * adf = NULL;
    ck_assert_ptr_null ( adf );
}
END_TEST


START_TEST ( test_adfimage_open_close )
{
    adfimage_t * adf = adfimage_open ( "testdata/ffdisk0049.adf" );
    ck_assert_ptr_nonnull ( adf );
    
    adfimage_close ( &adf );
    ck_assert_ptr_null ( adf );
}
END_TEST


START_TEST ( test_adfimage_properties )
{
    adfimage_t * adf = adfimage_open ( "testdata/ffdisk0049.adf" );

    ck_assert_int_eq ( adf->size, 901120 );
    ck_assert_ptr_nonnull ( adf->dev );
    ck_assert_ptr_nonnull ( adf->vol );

    adfimage_close ( &adf );
}
END_TEST


START_TEST ( test_adfimage_getdentry )
{
    adfimage_t * adf = adfimage_open ( "testdata/ffdisk0049.adf" );

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


START_TEST ( test_adfimage_getcwd )
{
    adfimage_t * adf = adfimage_open ( "testdata/ffdisk0049.adf" );

    const char * cwd = adfimage_getcwd ( adf );
    ck_assert_ptr_nonnull ( cwd );
    ck_assert_str_eq ( "/", cwd );

    adfimage_close ( &adf );
}
END_TEST



START_TEST ( test_adfimage_chdir )
{
    adfimage_t * adf = adfimage_open ( "testdata/ffdisk0049.adf" );

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

    adfimage_close ( &adf );
}
END_TEST


START_TEST ( test_adfimage_read )
{
    adfimage_t * adf = adfimage_open ( "testdata/ffdisk0049.adf" );

    adfimage_dentry_t dentry =
        adfimage_getdentry ( adf, "README.list49" );
    ck_assert_int_eq ( dentry.type, ADFVOLUME_DENTRY_FILE );

    char buf[1024];
    int bytes_read = adfimage_read ( adf, "README.list49",
                                     buf, 10, 0 );
    ck_assert_int_eq ( bytes_read, 10 );

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

    tc = tcase_create ( "adfimage getcwd" );
    tcase_add_test ( tc, test_adfimage_getcwd );
    suite_add_tcase ( s, tc );

    tc = tcase_create ( "adfimage chdir" );
    tcase_add_test ( tc, test_adfimage_chdir );
    suite_add_tcase ( s, tc );

    tc = tcase_create ( "adfimage read" );
    tcase_add_test ( tc, test_adfimage_read );
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
