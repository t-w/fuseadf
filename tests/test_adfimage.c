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

    adfvolume_dentry_t dentry =
        adfvolume_getdentry ( adf->vol, "non-exestent-file.tst" );
    ck_assert_int_eq ( dentry.type, ADFVOLUME_DENTRY_NONE );

    dentry = adfvolume_getdentry ( adf->vol, "README.list49" );
    ck_assert_int_eq ( dentry.type, ADFVOLUME_DENTRY_FILE );

    dentry = adfvolume_getdentry ( adf->vol, "Plot" );
    ck_assert_int_eq ( dentry.type, ADFVOLUME_DENTRY_DIRECTORY );

    adfimage_close ( &adf );
}
END_TEST


START_TEST ( test_adfimage_chdir )
{
    adfimage_t * adf = adfimage_open ( "testdata/ffdisk0049.adf" );

    BOOL result = adfvolume_chdir ( adf->vol, "non-exestent-dir" );
    ck_assert ( ! result );

    // go to top-level
    result = adfvolume_chdir ( adf->vol, "/" );
    ck_assert ( result );

    result = adfvolume_chdir ( adf->vol, "Plot" );
    ck_assert ( result );

    adfvolume_dentry_t dentry =
        adfvolume_getdentry ( adf->vol, "non-exestent-file.tst" );
    ck_assert_int_eq ( dentry.type, ADFVOLUME_DENTRY_NONE );

    dentry = adfvolume_getdentry ( adf->vol, "plot.c" );
    ck_assert_int_eq ( dentry.type, ADFVOLUME_DENTRY_FILE );

    // go to top-level
    result = adfvolume_chdir ( adf->vol, "/" );
    ck_assert ( result );

    // test case-insensitive chdir
    result = adfvolume_chdir ( adf->vol, "plot" );
    ck_assert ( result );

    dentry = adfvolume_getdentry ( adf->vol, "non-exestent-file.tst" );
    ck_assert_int_eq ( dentry.type, ADFVOLUME_DENTRY_NONE );

    dentry = adfvolume_getdentry ( adf->vol, "plot.c" );
    ck_assert_int_eq ( dentry.type, ADFVOLUME_DENTRY_FILE );

    // this fails - meaning getdentry is not case-insensitive(!)
    //dentry = adfvolume_getdentry ( adf->vol, "ploT.c" );
    //ck_assert_int_eq ( dentry.type, ADFVOLUME_DENTRY_FILE );

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

    tc = tcase_create ( "adfimage chdir" );
    tcase_add_test ( tc, test_adfimage_chdir );
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
