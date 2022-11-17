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
