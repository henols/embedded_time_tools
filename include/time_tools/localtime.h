/*
 * © 2016 Henrik Olsson, henols@gmail.com
 *
 * localtime.h
 *
 *  Created on: 11 okt. 2016
 *      Author: Henrik
 */

#ifndef LOCALTIME_H_
#define LOCALTIME_H_

#include <limits.h>
#include <time.h>

#define YEARSPERREPEAT		400	/* years before a Gregorian repeat */
#define SECSPERREPEAT		((long long) YEARSPERREPEAT * (long long) AVGSECSPERYEAR)
#define AVGSECSPERYEAR		31556952L

#define SECSPERMIN	60
#define MINSPERHOUR	60
#define HOURSPERDAY	24
#define DAYSPERWEEK	7
#define DAYSPERNYEAR	365
#define DAYSPERLYEAR	366
#define SECSPERHOUR	(SECSPERMIN * MINSPERHOUR)
#define SECSPERDAY	((long) SECSPERHOUR * HOURSPERDAY)
#define MONSPERYEAR	12

#define TM_SUNDAY	0
#define TM_MONDAY	1
#define TM_TUESDAY	2
#define TM_WEDNESDAY	3
#define TM_THURSDAY	4
#define TM_FRIDAY	5
#define TM_SATURDAY	6

#define TM_JANUARY	0
#define TM_FEBRUARY	1
#define TM_MARCH	2
#define TM_APRIL	3
#define TM_MAY		4
#define TM_JUNE		5
#define TM_JULY		6
#define TM_AUGUST	7
#define TM_SEPTEMBER	8
#define TM_OCTOBER	9
#define TM_NOVEMBER	10
#define TM_DECEMBER	11

#define TM_YEAR_BASE	1900

#define EPOCH_YEAR	1970
#define EPOCH_WDAY	TM_THURSDAY

#define TYPE_BIT(type)	(sizeof (type) * CHAR_BIT)
#define TYPE_SIGNED(type) (((type) -1) < 0)

#define INT_STRLEN_MAXIMUM(type) \
	((TYPE_BIT(type) - TYPE_SIGNED(type)) * 302 / 1000 + \
	1 + TYPE_SIGNED(type))

//typedef long long intmax_t;
//typedef unsigned long long uintmax_t;

typedef struct zone_info {
	char short_name[4];
	int offset;
	int dst_offset;
	int dst_times[20];
	int nr_of_dst_times;
	char name[32];
} zone_info_t;

char * tt_asctime_r(register const struct tm *timeptr, char *buf);

size_t tt_strftime(char *s, size_t maxsize, const char *format, const struct tm *t);

char *tt_ctime_r(const time_t *timep, char *buf);

struct tm *tt_localtime(const time_t timep, struct tm *tmp);

int tt_time_is_valid(struct tm * tm);

void tt_time_set_zone_info(zone_info_t * zone_info);

int tt_time_build_zone_info(zone_info_t * tz_info, const char * bytes, const int len);

int tt_time_lookup_zone_info(zone_info_t * tz_info, char * key);

struct tm * tt_timesub(const time_t time, int offset, struct tm *tmp);

zone_info_t * tt_time_get_zone_info(void);
double tt_difftime(time_t time1, time_t time0);

int tt_gmt_offset(zone_info_t * zone_info, const time_t t);

#define isleap(y) (((y) % 4) == 0 && (((y) % 100) != 0 || ((y) % 400) == 0))

#define isleap_sum(a, b) isleap((a) % 400 + (b) % 400)

#endif /* LOCALTIME_H_ */
