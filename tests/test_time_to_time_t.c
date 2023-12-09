#include <check.h>
#include <stdlib.h>
//#include <sys/time.h>
#include <time.h>

#include "../src/adffs_util.h"


START_TEST ( test_check_framework )
{
    ck_assert ( 1 );
}
END_TEST

extern long timezone;  // seconds West of UTC

START_TEST ( test_epoch_gm )
{
    adffs_util_init();
    const time_t epoch = 0;

    struct tm epoch_tm;
    gmtime_r ( &epoch, &epoch_tm );
    ck_assert_int_eq ( epoch_tm.tm_year, 70 );
    ck_assert_int_eq ( epoch_tm.tm_mon,   0 );
    ck_assert_int_eq ( epoch_tm.tm_mday,  1 );
    ck_assert_int_eq ( epoch_tm.tm_hour, 0 );
    ck_assert_int_eq ( epoch_tm.tm_min,  0 );
    ck_assert_int_eq ( epoch_tm.tm_sec,  0 );

    time_t epoch_calculated = gmtime_to_time_t (
        1970, 1, 1,
        0,
        0, 0 );
    ck_assert_int_eq ( epoch, epoch_calculated );
}
END_TEST


START_TEST ( test_now_gm )
{
    adffs_util_init();
    time_t now_orig = time ( NULL );   // UTC

    //struct timeval tv;
    //gettimeofday ( &tv, NULL );

    struct tm now_tm;
    //struct tm *gmtime_r(const time_t *timep, struct tm *result);
    //struct tm *localtime_r(const time_t *timep, struct tm *result);
    //localtime_r ( &now_orig, &now_tm );
    gmtime_r ( &now_orig, &now_tm );

    time_t now_2 = gmtime_to_time_t ( now_tm.tm_year + 1900,
                                      now_tm.tm_mon + 1,
                                      now_tm.tm_mday,
                                      now_tm.tm_hour,
                                      now_tm.tm_min,
                                      now_tm.tm_sec );
    ck_assert_int_eq ( now_orig, now_2 );
    //ck_assert ( now_orig == now_2 );
}
END_TEST


START_TEST ( test_epoch_local )
{
    adffs_util_init();
    time_t epoch = 0 + timezone;
    //time_t epoch = 0;

    struct tm epoch_tm;
    //epoch_tm.tm_isdst = daylight;
    //struct tm *gmtime_r(const time_t *timep, struct tm *result);
    //struct tm *localtime_r(const time_t *timep, struct tm *result);
    localtime_r ( &epoch, &epoch_tm );
    //gmtime_r ( &epoch, &epoch_tm );

    int timezone_hours = (int) ( timezone / 3600 );  // timezone as +/- hours vs UTC

    ck_assert_int_eq ( epoch_tm.tm_year, 70 );
    ck_assert_int_eq ( epoch_tm.tm_mon,   0 );
    ck_assert_int_eq ( epoch_tm.tm_mday,  1 );

                       // mktime is inverted function for localtime()!!!
                       // ( so eg. for CET is +1 )
    ck_assert_int_eq ( epoch_tm.tm_hour,
                       //-timezone_hours );
                       0 );
    ck_assert_int_eq ( epoch_tm.tm_hour, 0 );
    ck_assert_int_eq ( epoch_tm.tm_min,  0 );
    ck_assert_int_eq ( epoch_tm.tm_sec,  0 );

    time_t epoch_calculated = localtime_to_time_t (
        1970, 1, 1,
        //-timezone_hours,  // same thing as with above - epoch for eg. CET is UTC+1)
        0,
        0, 0 );

    ck_assert_int_eq ( epoch, epoch_calculated );
}
END_TEST


START_TEST ( test_now_local )
{
    adffs_util_init();
    time_t now_orig = time ( NULL );   // UTC

    //struct timeval tv;
    //gettimeofday ( &tv, NULL );

    struct tm now_tm;
    now_tm.tm_isdst = daylight;
    //struct tm *gmtime_r(const time_t *timep, struct tm *result);
    //struct tm *localtime_r(const time_t *timep, struct tm *result);
    localtime_r ( &now_orig, &now_tm );
    //gmtime_r ( &now_orig, &now_tm );

    int timezone_hours = (int) ( timezone / 3600 );  // timezone as +/- hours vs UTC
    time_t now_2 = localtime_to_time_t ( now_tm.tm_year + 1900,
                                         now_tm.tm_mon + 1,
                                         now_tm.tm_mday,
                                         now_tm.tm_hour,// - timezone_hours,
                                         now_tm.tm_min,
                                         now_tm.tm_sec );
    ck_assert_int_eq ( now_orig, now_2 );
    //ck_assert ( now_orig == now_2 );
}
END_TEST


Suite * time_suite ( void )
{
    Suite * s = suite_create ( "time" );
    
    TCase * tc = tcase_create ( "check framework" );
    tcase_add_test ( tc, test_check_framework );
    suite_add_tcase ( s, tc );

    tc = tcase_create ( "time converting epoch gm" );
    tcase_add_test ( tc, test_epoch_gm );
    suite_add_tcase ( s, tc );

    tc = tcase_create ( "time converting epoch local" );
    tcase_add_test ( tc, test_epoch_local );
    suite_add_tcase ( s, tc );

    tc = tcase_create ( "time converting now gm" );
    tcase_add_test ( tc, test_now_gm );
    suite_add_tcase ( s, tc );

    tc = tcase_create ( "time converting now local" );
    tcase_add_test ( tc, test_now_local );
    suite_add_tcase ( s, tc );

    return s;
}


int main ( void )
{
    Suite * s = time_suite();
    SRunner * sr = srunner_create ( s );

    srunner_run_all ( sr, CK_VERBOSE ); //CK_NORMAL );
    int number_failed = srunner_ntests_failed ( sr );
    srunner_free ( sr );
    return ( number_failed == 0 ) ?
        EXIT_SUCCESS :
        EXIT_FAILURE;
}
