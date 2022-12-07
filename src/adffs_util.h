
#ifndef __ADFFS_UTIL_H__
#define __ADFFS_UTIL_H__

#include <time.h>

unsigned long time_to_secs_from_epoch1 ( int      y,
                                        unsigned m,
                                        unsigned d );

time_t time_to_time_t ( int year,
                        int month,
                        int day,
                        int hour,
                        int min,
                        int sec );
#endif
