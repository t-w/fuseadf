
#include "adffs_util.h"

#include <string.h>

extern char *tzname[2];
//extern long timezone;
extern int daylight;

/*
// based on:
// https://stackoverflow.com/questions/7960318/math-to-convert-seconds-since-1970-into-date-and-vice-versa
unsigned long date_to_secs_from_epoch1 ( int      y,
                                         unsigned m,
                                         unsigned d )
{
    y -= m <= 2;
    const int era = (y >= 0 ? y : y-399) / 400;
    const unsigned yoe = (unsigned)(y - era * 400);      // [0, 399]
    const unsigned doy = (153*(m + (m > 2 ? -3 : 9)) + 2)/5 + d-1;  // [0, 365]
    const unsigned doe = yoe * 365 + yoe/4 - yoe/100 + doy;         // [0, 146096]

    //struct timespec ts = { 
    //    .tv_sec = (unsigned long) ( era * 146097 + (int) doe - 719468 ),
    //    .tv_nsec = 0
    //};
        //return era * 146097 + (int) doe - 719468;
    //return ts;
    return (unsigned long) ( era * 146097 + (int) doe - 719468 );
}


static long month_to_days ( int month )
{
    const static long days[12] = {
        31, 28, 31, 30, 31, 30,
        31, 31, 30, 31, 30, 31 };
    return days[month - 1];
}

static long days_since_1Jan ( int month )
{
    int days = 0;
    for ( int i = 0 ; i < month - 1 ; i++ )
        days += month_to_days(i);
}

// https://pubs.opengroup.org/onlinepubs/009695399/basedefs/xbd_chap04.html#tag_04_14
time_t time_to_secs_from_epoch2 ( int year,
                                  int month,
                                  int day,
                                  int hour,
                                  int min,
                                  int sec )
{
    long tm_year = year - 1900;
    long tm_yday = days_since_1Jan ( month ) + day;
    return //tm_sec + tm_min * 60 + tm_hour * 3600 +
        //tm_yday * 86400 +
        ( tm_year - 70 ) * 31536000 +
        ( ( tm_year - 69 ) / 4 ) * 86400 -
        ( ( tm_year - 1 ) / 100 ) * 86400 +
        ( ( tm_year + 299 ) / 400) * 86400;
}
*/

void adffs_util_init(void)
{
    tzset();
}

time_t gmtime_to_time_t ( const int year,
                          const int month,
                          const int day,
                          const int hour,
                          const int min,
                          const int sec )
{
    struct tm time_tm;
    memset ( &time_tm, 0, sizeof ( struct tm ) );
    time_tm.tm_year = year - 1900;
    time_tm.tm_mon  = month - 1;
    time_tm.tm_mday = day;
    time_tm.tm_hour = hour;
    time_tm.tm_min  = min;
    time_tm.tm_sec  = sec;
    //time_tm.tm_isdst = -1;
    time_tm.tm_isdst = 0;
    //time_tm.tm_isdst = daylight;

    return timegm ( &time_tm );
}


time_t localtime_to_time_t ( const int year,
                             const int month,
                             const int day,
                             const int hour,
                             const int min,
                             const int sec )
{
    struct tm time_tm;
    memset ( &time_tm, 0, sizeof ( struct tm ) );
    time_tm.tm_year = year - 1900;
    time_tm.tm_mon  = month - 1;
    time_tm.tm_mday = day;
    time_tm.tm_hour = hour;
    time_tm.tm_min  = min;
    time_tm.tm_sec  = sec;
    time_tm.tm_isdst = -1;
    //time_tm.tm_isdst = 0;
    //time_tm.tm_isdst = daylight;

    return mktime ( &time_tm );// + timezone;  // note that mktime is inverse function of localtime()!!!
}
