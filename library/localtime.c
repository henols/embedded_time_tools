/*
 * © 2016 Henrik Olsson, henols@gmail.com
 *
 * localtime.c
 *
 *  Created on: 11 okt. 2016
 *      Author: Henrik
 */

#include "time_tools/localtime.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/_timeval.h>


static const int mon_lengths[2][MONSPERYEAR] = { { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }, { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 } };

static const int year_lengths[2] = {
DAYSPERNYEAR, DAYSPERLYEAR };

zone_info_t * zone_info = NULL;

//char tzname[4] = { 'U', 'T', 'C', NULL, };

extern int tt_time_lookup_tz(zone_info_t * tz_info, char * key);

static struct tm * localsub(zone_info_t * zone_info, const time_t t, struct tm * const tmp);

/*
 ** Return the number of leap years through the end of the given year
 ** where, to make the math easy, the answer for year zero is defined as zero.
 */
static int leaps_thru_end_of(register const int y) {

	int rc;
//	FUNC_ENTRY;
	rc = (y >= 0) ? (y / 4 - y / 100 + y / 400) : -(leaps_thru_end_of(-(y + 1)) + 1);

//	FUNC_EXIT_RC(rc);
	return rc;
}

static int increment_overflow(int *ip, int j) {
	int rc = 0;
	register int const i = *ip;

//	FUNC_ENTRY;
	/*
	 ** If i >= 0 there can only be overflow if i + j > INT_MAX
	 ** or if j > INT_MAX - i; given i >= 0, INT_MAX - i cannot overflow.
	 ** If i < 0 there can only be overflow if i + j < INT_MIN
	 ** or if j < INT_MIN - i; given i < 0, INT_MIN - i cannot overflow.
	 */
	if ((i >= 0) ? (j > INT_MAX - i) : (j < INT_MIN - i)) {
		rc = 1;
		goto end;
	}
	*ip += j;

	end:
//	FUNC_EXIT_RC(rc);
	return rc;
}

struct tm * tt_timesub(const time_t time, int offset, struct tm *tmp) {

	 time_t tdays;
	 int idays; /* unsigned would be so 2003 */
	 long long rem;
	int y;
	 const int * ip;
	 int hit;
//	FUNC_ENTRY;
	hit = 0;
//	while (--i >= 0) {
//		lp = &sp->lsis[i];
//		if (*timep >= lp->ls_trans) {
//			if (*timep == lp->ls_trans) {
//				hit = ((i == 0 && lp->ls_corr > 0) || lp->ls_corr > sp->lsis[i - 1].ls_corr);
//				if (hit)
//					while (i > 0 && sp->lsis[i].ls_trans == sp->lsis[i - 1].ls_trans + 1 && sp->lsis[i].ls_corr == sp->lsis[i - 1].ls_corr + 1) {
//						++hit;
//						--i;
//					}
//			}
//			corr = lp->ls_corr;
//			break;
//		}
//	}
	y = EPOCH_YEAR;
	tdays = time / SECSPERDAY;
	rem = time % SECSPERDAY;
	while (tdays < 0 || tdays >= year_lengths[isleap(y)]) {
		int newy;
		register time_t tdelta;
		register int idelta;
		register int leapdays;

		tdelta = tdays / DAYSPERLYEAR;
		if (!((!TYPE_SIGNED(time_t) || INT_MIN <= tdelta) && tdelta <= INT_MAX)) {
			tmp = NULL;
			goto error;
		}
		idelta = tdelta;
		if (idelta == 0)
			idelta = (tdays < 0) ? -1 : 1;
		newy = y;
		if (increment_overflow(&newy, idelta)) {
			tmp = NULL;
			goto error;
		}
		leapdays = leaps_thru_end_of(newy - 1) - leaps_thru_end_of(y - 1);
		tdays -= ((time_t) newy - y) * DAYSPERNYEAR;
		tdays -= leapdays;
		y = newy;
	}
	/*
	 ** Given the range, we can now fearlessly cast...
	 */
	idays = tdays;
	rem += offset;
	while (rem < 0) {
		rem += SECSPERDAY;
		--idays;
	}
	while (rem >= SECSPERDAY) {
		rem -= SECSPERDAY;
		++idays;
	}
	while (idays < 0) {
		if (increment_overflow(&y, -1)) {
			tmp = NULL;
			goto error;

		}
		idays += year_lengths[isleap(y)];
	}
	while (idays >= year_lengths[isleap(y)]) {
		idays -= year_lengths[isleap(y)];
		if (increment_overflow(&y, 1)) {
			tmp = NULL;
			goto error;
		}
	}
	tmp->tm_year = y;
	if (increment_overflow(&tmp->tm_year, -TM_YEAR_BASE)) {
		tmp = NULL;
		goto error;
	}
	tmp->tm_yday = idays;
	/*
	 ** The "extra" mods below avoid overflow problems.
	 */
	tmp->tm_wday = EPOCH_WDAY + ((y - EPOCH_YEAR) % DAYSPERWEEK) * (DAYSPERNYEAR % DAYSPERWEEK) + leaps_thru_end_of(y - 1) - leaps_thru_end_of(EPOCH_YEAR - 1) + idays;
	tmp->tm_wday %= DAYSPERWEEK;
	if (tmp->tm_wday < 0)
		tmp->tm_wday += DAYSPERWEEK;
	tmp->tm_hour = (int) (rem / SECSPERHOUR);
	rem %= SECSPERHOUR;
	tmp->tm_min = (int) (rem / SECSPERMIN);
	/*
	 ** A positive leap second requires a special
	 ** representation. This uses "... ??:59:60" et seq.
	 */
	tmp->tm_sec = (int) (rem % SECSPERMIN) + hit;
	ip = mon_lengths[isleap(y)];
	for (tmp->tm_mon = 0; idays >= ip[tmp->tm_mon]; ++(tmp->tm_mon))
		idays -= ip[tmp->tm_mon];
	tmp->tm_mday = (int) (idays + 1);
	tmp->tm_isdst = 0;
#ifdef TM_GMTOFF
	tmp->TM_GMTOFF = offset;
#endif /* defined TM_GMTOFF */

	error:

	return tmp;
}

int tt_time_build_zone_info(zone_info_t * tz_info, const char * bytes, int len) {
	int size = sizeof(zone_info_t);
	if (tz_info) {
		memset(tz_info, 0, size);
	} else {
		return 0;
	}
	if (len > size - 4) {
		len = size - 4;
	}
	memcpy(tz_info, bytes, len);
	if (len > 8) {
		tz_info->nr_of_dst_times = (len - 12) / 8;
	}

	printf(" Name: %s\n", tz_info->short_name);
	printf(" Offset: %dsec, Nr of DST: %d\n", tz_info->offset, tz_info->nr_of_dst_times);
	if(tz_info->nr_of_dst_times>0){
		struct tm end_time;
		char date[20] = {0};
		tt_localtime(tz_info->dst_times[tz_info->nr_of_dst_times *2 -1], &end_time);
		tt_strftime(date, 20, "%F", &end_time);
		printf(" Last DTS info at: %s\n", date);
	}

	return 1;

}

void tt_time_set_zone_info(zone_info_t * tz_info) {
	int size = sizeof(zone_info_t);
	if (zone_info) {
		memset(zone_info, 0, size);
	} else {
		zone_info = (zone_info_t *) malloc(size);
		memset(zone_info, 0, size);
	}
	memcpy(zone_info, tz_info, size);
//	memcpy(tzname, tz_info->short_name, 4);

	printf(" Setting time zone info: %s, %s\n", zone_info->name, zone_info->short_name);
}

zone_info_t * tt_time_get_zone_info() {
	return zone_info;
}

int tt_time_lookup_zone_info(zone_info_t * tz_info, char * key){
	int rc = tt_time_lookup_tz(tz_info, key);
	if(rc){
		strcpy(tz_info->name, key);
	}
	return rc;
}

struct tm *tt_localtime(const time_t time, struct tm *tmp) {
	return localsub(zone_info, time, tmp);
}

int tt_time_is_valid(struct tm * tm) {
	int rc = 1;
	if (tm->tm_year + 1900 < 2016) {
		rc = 0;
	}
	return rc;
}

static int gmt_offset(zone_info_t * zone_info, const time_t t, int * is_dst ){
	int gmt_offset = 0;
	if (zone_info) {
		gmt_offset = zone_info->offset;
		if (zone_info->dst_times) {
			int i;
			for (i = 0; i < zone_info->nr_of_dst_times; i++) {
				if (t >= (int) zone_info->dst_times[i] && t < (int) zone_info->dst_times[++i]) {
					gmt_offset += zone_info->dst_offset;
					*is_dst = 1;
					break;
				}
			}
		}
	}
	return gmt_offset;
}

int tt_gmt_offset(zone_info_t * zone_info, const time_t t) {
	int is_dst = 0;
	return gmt_offset(zone_info, t, &is_dst);
}

/*ARGSUSED*/
static struct tm * localsub(zone_info_t * zone_info, const time_t t, struct tm * const tmp) {
	struct tm * result;

	int _gmt_offset = 0;
	int is_dst = 0;

	_gmt_offset = gmt_offset(zone_info, t, &is_dst);
	result = tt_timesub(t, _gmt_offset, tmp);
	if (result) {
		result->tm_isdst = is_dst;
	}

	return result;
}


#if 0

//--------------------------------------------------------------------------------
#define WRONG NULL
#define TZ_MAX_TYPES 20
#define TZ_MAX_TIMES 20
#define int_fast32_t long

struct state {
	int		leapcnt;
	int		timecnt;
	int		typecnt;
	int		charcnt;
	bool		goback;
	bool		goahead;
	time_t		ats[TZ_MAX_TIMES];
	unsigned char	types[TZ_MAX_TIMES];
	struct ttinfo	ttis[TZ_MAX_TYPES];
	char		chars[BIGGEST(BIGGEST(TZ_MAX_CHARS + 1, sizeof gmt),
				(2 * (MY_TZNAME_MAX + 1)))];
	struct lsinfo	lsis[TZ_MAX_LEAPS];
	int		defaulttype; /* for early times or if no transitions */
};


#define bool int
#define false 0
#define true 1

static time_t
time2(struct tm * const	tmp,
      struct tm *(*funcp)( zone_info_t * ,time_t const *,
			   struct tm *),
      const int_fast32_t offset,
      bool *okayp)
{
	time_t	t;

	/*
	** First try without normalization of seconds
	** (in case tm_sec contains a value associated with a leap second).
	** If that fails, try with normalization of seconds.
	*/
	t = time2sub(tmp, funcp, offset, okayp, false);
	return *okayp ? t : time2sub(tmp, funcp, offset, okayp, true);
}

static time_t
time1(struct tm *const tmp,
      struct tm *(*funcp) (zone_info_t * ,time_t const *,
			    struct tm *),
      const int_fast32_t offset)
{
	register time_t			t;
	register int			samei, otheri;
	register int			sameind, otherind;
	register int			i;
	register int			nseen;
	char				seen[TZ_MAX_TYPES];
	unsigned char			types[TZ_MAX_TYPES];
	bool				okay;

	if (tmp == NULL) {
		return WRONG;
	}
	if (tmp->tm_isdst > 1)
		tmp->tm_isdst = 1;
	t = time2(tmp, funcp, offset, &okay);
	if (okay)
		return t;
	if (tmp->tm_isdst < 0)
#ifdef PCTS
		/*
		** POSIX Conformance Test Suite code courtesy Grant Sullivan.
		*/
		tmp->tm_isdst = 0;	/* reset to std and try again */
#else
		return t;
#endif /* !defined PCTS */
	/*
	** We're supposed to assume that somebody took a time of one type
	** and did some math on it that yielded a "struct tm" that's bad.
	** We try to divine the type they started from and adjust to the
	** type they need.
	*/
	if (sp == NULL)
		return WRONG;
	for (i = 0; i < sp->typecnt; ++i)
		seen[i] = false;
	nseen = 0;
	for (i = sp->timecnt - 1; i >= 0; --i)
		if (!seen[sp->types[i]]) {
			seen[sp->types[i]] = true;
			types[nseen++] = sp->types[i];
		}
	for (sameind = 0; sameind < nseen; ++sameind) {
		samei = types[sameind];
		if (sp->ttis[samei].tt_isdst != tmp->tm_isdst)
			continue;
		for (otherind = 0; otherind < nseen; ++otherind) {
			otheri = types[otherind];
			if (sp->ttis[otheri].tt_isdst == tmp->tm_isdst)
				continue;
			tmp->tm_sec += sp->ttis[otheri].tt_gmtoff -
					sp->ttis[samei].tt_gmtoff;
			tmp->tm_isdst = !tmp->tm_isdst;
			t = time2(tmp, funcp, sp, offset, &okay);
			if (okay)
				return t;
			tmp->tm_sec -= sp->ttis[otheri].tt_gmtoff - sp->ttis[samei].tt_gmtoff;
			tmp->tm_isdst = !tmp->tm_isdst;
		}
	}
	return WRONG;
}

static time_t
mktime_tzname(struct tm *tmp, bool setname)
{
    return time1(tmp, localsub, setname);
}

#if NETBSD_INSPIRED

time_t
mktime_z(struct state *sp, struct tm *tmp)
{
  return mktime_tzname(sp, tmp, false);
}

#endif

time_t
mktime(struct tm *tmp)
{
  time_t t;
  t = mktime_tzname( tmp, true);
  return t;
}

#ifdef STD_INSPIRED

time_t
timelocal(struct tm *tmp)
{
	if (tmp != NULL)
		tmp->tm_isdst = -1;	/* in case it wasn't initialized */
	return mktime(tmp);
}

time_t
timegm(struct tm *tmp)
{
  return timeoff(tmp, 0);
}

time_t
timeoff(struct tm *tmp, long offset)
{
  if (tmp)
    tmp->tm_isdst = 0;
  gmtcheck();
  return time1(tmp, gmtsub, gmtptr, offset);
}

#endif /* defined STD_INSPIRED */

/*
** XXX--is the below the right way to conditionalize??
*/

#ifdef STD_INSPIRED

/*
** IEEE Std 1003.1-1988 (POSIX) legislates that 536457599
** shall correspond to "Wed Dec 31 23:59:59 UTC 1986", which
** is not the case if we are accounting for leap seconds.
** So, we provide the following conversion routines for use
** when exchanging timestamps with POSIX conforming systems.
*/

static int_fast64_t
leapcorr(struct state const *sp, time_t t)
{
	register struct lsinfo const *	lp;
	register int			i;

	i = sp->leapcnt;
	while (--i >= 0) {
		lp = &sp->lsis[i];
		if (t >= lp->ls_trans)
			return lp->ls_corr;
	}
	return 0;
}


#endif
#endif
