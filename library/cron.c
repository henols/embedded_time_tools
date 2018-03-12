/*
 * © 2017 Henrik Olsson, henols@gmail.com
 *
 * cron.c
 *
 *  Created on: 20 mars 2017
 *      Author: Henrik
 */

#include "time_tools/cron.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "time_tools/localtime.h"

typedef struct cron_entry {
	struct cron_entry *next;
	char *mn;
	char *hr;
	char *day;
	char *mon;
	char *wkd;
	char *cmd;
	void * user_data;
	CRON_JOB job;
} CRON;

static void wakeup(struct tm * tm);

static CRON *head = NULL; /* crontab entry pointers       */

void tt_cron_refresh() {
	time_t t;
	static time_t min = 0;
	time_t tmp_min = 0;
	struct tm time;
//	vm_get_rtc(&t); TODO get some time
	t= 0;
	tmp_min = t / 60;
	if (tmp_min > min && (t % 60 == 0) && tt_localtime(t, &time)) {
		min = tmp_min;
		wakeup(&time);
	}
}

/*
 * minutes hours   day of mounth  mounth  day of week
 * (0-59)  (0-23)  (1-31)         (1-12)  (0-6)
 *
 *   min hr dat mo day   command
 *    *  *   *  *   *    /usr/bin/date >/dev/tty0   #print date every minute
 *    0  *   *  *   *    /usr/bin/date >/dev/tty0   #print date on the hour
 *   30  4   *  *  1-5   /bin/backup /dev/fd1       #do backup Mon-Fri at 0430
 *   30 19   *  *  1,3,5 /etc/backup /dev/fd1       #Mon, Wed, Fri at 1930
 *    0  9  25 12   *    /usr/bin/sing >/dev/tty0   #Xmas morning at 0900 only
 */
void tt_cron_add_job(char *mn, char *hr, char *day, char *mon, char *wkd, char *cmd, CRON_JOB job, void * user_data) {
	CRON *this_entry = NULL, *entry_ptr = NULL;
	int i = 0;

	if (head == NULL) {
		head = (CRON *) malloc(sizeof(CRON));
		head->next = NULL;
		head->job = NULL;
	}

	entry_ptr = head;
	while (entry_ptr->next) {
		i++;
		if (strcmp(entry_ptr->cmd, cmd) == 0) {
			break;
		}
		entry_ptr = entry_ptr->next;
	}

	entry_ptr->mn = strdup(mn);
	entry_ptr->hr = strdup(hr);
	entry_ptr->day = strdup(day);
	entry_ptr->mon = strdup(mon);
	entry_ptr->wkd = strdup(wkd);

	entry_ptr->cmd = strdup(cmd);
	entry_ptr->job = job;
	entry_ptr->user_data = user_data;

	if (entry_ptr->next == NULL) {
		this_entry = (CRON *) malloc(sizeof(CRON));
		this_entry->next = NULL;
		this_entry->job = NULL;
		entry_ptr->next = this_entry;
	}
	printf(" Add CRON job: %s, nr: %d\n", entry_ptr->cmd, i);
}

int tt_cron_remove_job(char *cmd) {
	CRON *entry_ptr = NULL;
	int rc = 0;
	entry_ptr = head;
	if (strcmp(entry_ptr->cmd, cmd) == 0) {
		head = entry_ptr->next;
		if (entry_ptr->cmd) {
			free(entry_ptr->cmd);
			entry_ptr->cmd = NULL;
		}
		if (entry_ptr->mn) {
			free(entry_ptr->mn);
			entry_ptr->mn = NULL;
		}
		if (entry_ptr->hr) {
			free(entry_ptr->hr);
			entry_ptr->hr = NULL;
		}
		if (entry_ptr->day) {
			free(entry_ptr->day);
			entry_ptr->day = NULL;
		}
		if (entry_ptr->mon) {
			free(entry_ptr->mon);
			entry_ptr->mon = NULL;
		}
		if (entry_ptr->wkd) {
			free(entry_ptr->wkd);
			entry_ptr->wkd = NULL;
		}

		free(entry_ptr);
		rc = 1;
		goto exit;
	}

	while (entry_ptr->next) {
		if (strcmp(entry_ptr->next->cmd, cmd) == 0) {
			CRON *tmp_ptr = NULL;
			tmp_ptr = entry_ptr->next;
			entry_ptr->next = tmp_ptr->next;
			free(entry_ptr->cmd);
			entry_ptr->cmd = NULL;
			free(tmp_ptr);
			rc = 1;
			goto exit;
		}
		entry_ptr = entry_ptr->next;
	}
	exit: //
	printf(" Remove CRON job: %s, found %d\n", cmd, rc);

	return rc;
}

/*
 * This routine will match the left string with the right number.
 *
 * The string can contain the following syntax *
 *      *               This will return 1 for any number
 *      x,y [,z, ...]   This will return 1 for any number given.
 *      x-y             This will return 1 for any number within
 *                      the range of x thru y.
 */
static int match(char *left, int right)
//   register char *left;
//   register int right;
{
	register int n;
	register char c;

	n = 0;
	if (!strcmp(left, "*"))
		return (1);

	while ((c = *left++) && (c >= '0') && (c <= '9'))
		n = (n * 10) + c - '0';

	switch (c) {
	case '\0':
		return (right == n);
		/*NOTREACHED*/
		break;
	case ',':
		if (right == n)
			return (1);
		do {
			n = 0;
			while ((c = *left++) && (c >= '0') && (c <= '9'))
				n = (n * 10) + c - '0';

			if (right == n)
				return (1);
		} while (c == ',');
		return (0);
		/*NOTREACHED*/
		break;
	case '-':
		if (right < n)
			return (0);
		n = 0;
		while ((c = *left++) && (c >= '0') && (c <= '9'))
			n = (n * 10) + c - '0';
		return (right <= n);
		/*NOTREACHED*/
		break;
	default:
		break;
	}
	return (0);
}

#define TIME_LEN 21
void wakeup(struct tm * tm) {
	CRON *this_entry;

	this_entry = head;

	while (this_entry->next) {
		if (match(this_entry->mn, tm->tm_min) && match(this_entry->hr, tm->tm_hour) && match(this_entry->day, tm->tm_mday) && match(this_entry->mon, tm->tm_mon + 1) && match(this_entry->wkd, tm->tm_wday)) {
			char time_str[TIME_LEN];
			tt_strftime(time_str, TIME_LEN, "%Y-%m-%d %T", tm);

			printf(" Execute CRON job %s: %s\n", this_entry->cmd, time_str);
			if (this_entry->job) {
				this_entry->job(tm, this_entry->user_data);
			}
		}
		this_entry = this_entry->next;
	}
}
