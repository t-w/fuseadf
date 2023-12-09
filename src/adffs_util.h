
#ifndef __ADFFS_UTIL_H__
#define __ADFFS_UTIL_H__

#include <time.h>

//unsigned long time_to_secs_from_epoch1 ( int      y,
//                                        unsigned m,
//                                        unsigned d );

void adffs_util_init(void);

time_t gmtime_to_time_t ( const int year,
                          const int month,
                          const int day,
                          const int hour,
                          const int min,
                          const int sec );

time_t localtime_to_time_t ( const int year,
                             const int month,
                             const int day,
                             const int hour,
                             const int min,
                             const int sec );
#endif
