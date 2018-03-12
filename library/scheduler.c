/*
 * © 2017 Henrik Olsson, henols@gmail.com
 *
 * scheduler.c
 *
 *  Created on: 27 nov. 2017
 *      Author: Henrik
 */

#include "time_tools/scheduler.h"

#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "time_tools/cron.h"
#include "time_tools/localtime.h"

//#define DEBUG_ME
//#define TEST_ME

static void delete_time_slot(time_slot_t* slot);
static int validate_hour(int hour);
static int validate_minute(int minute);
static int compare_time(int hour_a, int minute_a, int hour_b, int minute_b);
static int compare_time_point_by_time(time_point_t * a, time_point_t * b);
static int is_day_between(int start, int end, int day);

static void set_time_point(time_point_t * point, char* day, int hour, int minute);
static void set_time_point_day(time_point_t * point, char* day);

time_slot_t* create_time_slot(char* day, int start_hour, int start_minute, int end_hour, int end_minute);
time_slot_t* clone_time_slot(time_slot_t* time_slot);

static int split_day(char *dest, char *arg);
static int increment_day(char * res, char *arg);

void remove_scheduled_jobs(schedule_t* schedule);


typedef struct schedule_holder {
	schedule_t * schedule;
	struct schedule_holder * child;
} schedule_holder_t;

static schedule_holder_t * schedules;

void schedule_cb(int enable) {
	printf(" Schedule enabled: %d\n", enable);
}

#ifdef DEBUG_ME
void print_schedule(schedule_t * schedule);
void print_time_slot(time_slot_t * slot);
void print_point(time_point_t * point);
#endif

void tt_scheduler_init(void) {
#ifdef TEST_ME
	schedule_t* schedule;
	time_t t;
	struct tm time;
#endif
	schedules = NULL;
#ifdef TEST_ME
	schedule = vm_malloc(sizeof(schedule_t));

	vm_get_rtc(&t);
	tt_localtime(t, &time);


	tt_scheduler_init_schedule(schedule, "Schedule Test\n", schedule_cb);
	printf(" Schedule: %s\n", schedule->name);
	printf(" schedule->schedule_cb: %p\n", schedule->schedule_cb);
//	tt_scheduler_solve(schedule, "0-2", 20, 00, 04, 00);
//	tt_scheduler_solve(schedule, "2", 22, 00, 04, 10);
//	tt_scheduler_solve(schedule, "4-0", 21, 00, 07, 10);
//	tt_scheduler_solve(schedule, "3-5", 13, 50, 13, 52);
//	tt_scheduler_solve(schedule, "5-3", 13, 50, 13, 52);
//	tt_scheduler_solve(schedule, "5", 13, 3, 13, 4);
	if (tt_scheduler_solve(schedule, "0-6", (time.tm_hour + 1) %24, time.tm_min+1, (time.tm_hour + 1) %24 , time.tm_min+2)) {
#ifdef DEBUG_ME
		print_schedule(schedule);
#endif
		tt_scheduler_apply(schedule);
#ifdef DEBUG_ME
		print_schedule(schedule);
#endif
	} else {
		tt_scheduler_free(schedule);
		free(schedule);
		printf(" Failed to solve schedule");
	}
#endif
}

void tt_scheduler_deinit(void) {
	schedule_holder_t * holder;
	schedule_holder_t * child;
	holder = schedules;
	while (holder) {
		child = holder->child;
		remove_scheduled_jobs(holder->schedule);
		tt_scheduler_free(holder->schedule);
		free(holder);
		holder = child;
	}
}

void tt_scheduler_init_schedule(schedule_t * schedule, char * name, schedule_callback schedule_cb) {
	schedule->name = strdup(name);
	schedule->schedule_cb = schedule_cb;
	schedule->time_slot = NULL;
}

void set_time_point_day(time_point_t * point, char* day) {
	if (point->day) {
		free(point->day);
	}
	point->day = strdup(day);
	point->day_num = -1;
	if (strlen(point->day) == 1) {
		point->day_num = *point->day - '0';
	}
}

void set_time_point(time_point_t * point, char* day, int hour, int minute) {
	point->hour = hour;
	point->minute = minute;
	set_time_point_day(point, day);
}

time_slot_t* clone_time_slot(time_slot_t* time_slot) {
	time_slot_t* clone;
	clone = malloc(sizeof(time_slot_t));
	clone->child = NULL;
	clone->start_time.day = NULL;
	clone->end_time.day = NULL;
	set_time_point(&clone->start_time, time_slot->start_time.day, time_slot->start_time.hour, time_slot->start_time.minute);
	set_time_point(&clone->end_time, time_slot->end_time.day, time_slot->end_time.hour, time_slot->end_time.minute);

	return clone;
}
void set_time_point_end_day(time_slot_t* time_slot, char* day);

time_slot_t* create_time_slot(char* day, int start_hour, int start_minute, int end_hour, int end_minute) {
	time_slot_t* time_slot;
	time_slot = malloc(sizeof(time_slot_t));
	time_slot->child = NULL;
	time_slot->start_time.day = NULL;
	time_slot->end_time.day = NULL;
	set_time_point(&time_slot->start_time, day, start_hour, start_minute);
	set_time_point(&time_slot->end_time, day, end_hour, end_minute);

	set_time_point_end_day(time_slot, day);

	return time_slot;
}

void set_time_point_end_day(time_slot_t* time_slot, char* day) {
	if (compare_time_point_by_time(&time_slot->start_time, &time_slot->end_time) < 0) {
		char* end_day;
		end_day = malloc(strlen(day) + 1);
		increment_day(end_day, day);
		set_time_point_day(&time_slot->end_time, end_day);
		free(end_day);
	} else {
		set_time_point_day(&time_slot->end_time, day);
	}
}

static int is_day_between(int start, int end, int day) {
	return (start < end ? (start <= day && end >= day) : (start >= day && end <= day));
}

static int in_range(time_slot_t * time_slot, time_point_t * point) {
	int i, j;
	time_point_t* start;
	time_point_t* end;
	int nr_start_days = 1;
	int nr_point_days = 1;
	char start_days[8] = { '\0' };
	char end_days[8] = { '\0' };
	char point_days[8] = { '\0' };

	start = &time_slot->start_time;
	end = &time_slot->end_time;

	if (start->day_num < 0) {
		nr_start_days = split_day(start_days, start->day);
		split_day(end_days, end->day);
	} else {
		start_days[0] = '0' + start->day_num;
		end_days[0] = '0' + end->day_num;
	}
	if (point->day_num < 0) {
		nr_point_days = split_day(point_days, point->day);
	} else {
		point_days[0] = '0' + point->day_num;
	}

	// Check that the day is in the range, if the day is equal to star or end compare the time.
	for (i = 0; i < nr_start_days; i++) {
		int s_day_num = start_days[i] - '0';
		int e_day_num = end_days[i] - '0';
		for (j = 0; j < nr_point_days; j++) {
			int p_day_num = point_days[j] - '0';
//			log_info(" between %c %c, point day: %c",start_days[i],end_days[i],point_days[j]);
			if (is_day_between(s_day_num, e_day_num, p_day_num) && //
					((s_day_num == p_day_num ? compare_time_point_by_time(start, point) >= 0 : 1) && //
					(e_day_num == p_day_num ? compare_time_point_by_time(end, point) <= 0 : 1))) {
				return 1;
			}
		}
	}
	return 0;
}

static int match_time_slots(time_slot_t * slot_a, time_slot_t * slot_b) {
	int consume = 0;
	int start_in_range = 0;
	int end_in_range = 0;

	time_point_t* start_point;
	time_point_t* end_point;

#ifdef DEBUG_ME
	printf(" match_time_slots");
#endif
	start_point = &slot_b->start_time;
	end_point = &slot_b->end_time;
	if (in_range(slot_a, start_point)) {
		// set slot b end time to slot a end time if slot b is earlier
#ifdef DEBUG_ME
		printf(" Consume: Start time is in range of slot a, D:%s T:%02d:%02d\n", start_point->day, start_point->hour, start_point->minute);
#endif
		start_in_range = 1;

		consume = 1;
	}
	if (in_range(slot_a, end_point)) {
		// set slot b start time to slot a start time if slot b is later
#ifdef DEBUG_ME
		printf(" Consume: End time is in range of slot a, D:%s T:%02d:%02d\n", end_point->day, end_point->hour, end_point->minute);
#endif
		end_in_range = 1;
		consume = 1;
	}

	if (start_in_range && in_range(slot_b, &slot_a->end_time)) {
#ifdef DEBUG_ME
		printf(" Set End time to D:%s T:%02d:%02d\n", end_point->day, end_point->hour, end_point->minute);
#endif
		set_time_point(&slot_a->end_time, end_point->day, end_point->hour, end_point->minute);
	}

	if (end_in_range && in_range(slot_b, &slot_a->start_time)) {
#ifdef DEBUG_ME
		printf(" Set Start time to D:%s T:%02d:%02d\n", start_point->day, start_point->hour, start_point->minute);
#endif
		set_time_point(&slot_a->start_time, start_point->day, start_point->hour, start_point->minute);
	}
	return consume;
}

// Add a time slot to the end of the list
int add_time_slot(time_slot_t* time_slot, time_slot_t* child_slot) {
	int rc = 0;
	while (time_slot && child_slot != NULL) {
		if (time_slot->child == NULL) {
			time_slot->child = child_slot;
			rc = 1;
			break;
		}
		time_slot = time_slot->child;
	}
	return rc;
}

void detach_time_slot(time_slot_t* current_slot, time_slot_t* child_slot) {
	while (current_slot != NULL && child_slot != NULL) {
		if (current_slot->child == child_slot) {
			current_slot->child = child_slot->child;
			child_slot->child = NULL;
#ifdef DEBUG_ME
			printf(" Child slot detached");
#endif
			break;
		}
		current_slot = current_slot->child;
	}
}

/**
 * Returns TRUE if time slot is added to the list
 * 	and FASLE if its consumed and shall be ignored
 */
static int solve(time_slot_t * time_slot) {
	time_slot_t * current_slot;
	time_slot_t * child_slot;
	current_slot = time_slot;
	if (!current_slot) {
		return 0;
	}
	start: //
	while (current_slot) {
		time_slot_t * child;
		child_slot = current_slot->child;
		while (child_slot) {
			child = child_slot->child;
			if (match_time_slots(current_slot, child_slot)) {
				detach_time_slot(current_slot, child_slot);
				delete_time_slot(child_slot);
				current_slot = time_slot;
				// Iterate until all time slots are solved
				goto start;
			}
			child_slot = child;
		}
		current_slot = current_slot->child;
	}
	return 1;
}

void split_time_slot(time_slot_t* time_slot) {
	time_slot_t* tmp_slot = NULL;
	time_slot_t* prev_child;
	char day[2] = { '\0' };
	int i = 0;
	char day_list[8];
	int nr_days;
	nr_days = split_day(day_list, time_slot->start_time.day);
	day[0] = day_list[i];
	prev_child = time_slot->child;
	time_slot->child = NULL;

	set_time_point_day(&time_slot->start_time, day);
	set_time_point_end_day(time_slot, day);

	for (i = 1; i < nr_days; i++) {
		time_slot_t* child_slot;
		tmp_slot = clone_time_slot(time_slot);
		day[0] = day_list[i];
		set_time_point_day(&time_slot->start_time, day);
		set_time_point_end_day(tmp_slot, day);
		child_slot = time_slot;
		while (child_slot != NULL) {
			if (child_slot->child == NULL) {
				child_slot->child = tmp_slot;
				break;
			}
			child_slot = child_slot->child;
		}
	}
	if (tmp_slot->child) {
		tmp_slot->child = prev_child;
	}
}

int tt_scheduler_solve(schedule_t* schedule, char* days, int start_hour, int start_minute, int end_hour, int end_minute) {
	time_slot_t* time_slot = NULL;
	time_slot_t* child_slot;
	char day_list[8] = { '\0' };
	char day[2] = { '\0' };
	int nr_days;
	int i = 0;

	if (!validate_hour(start_hour) || !validate_minute(start_minute) || !validate_hour(end_hour) || !validate_minute(end_minute)) {
		return 0;
	}

	if (compare_time(start_hour, start_minute, end_hour, end_minute) == 0) {
		return 0;
	}

	printf(" Solve: D:%s T:%02d:%02d, End: T:%02d:%02d\n", days, start_hour, start_minute, end_hour, end_minute);

	nr_days = split_day(day_list, days);
	if (nr_days <= 0) {
		return 0;
	}
	day[0] = day_list[i];

	time_slot = create_time_slot(day, start_hour, start_minute, end_hour, end_minute);
	child_slot = time_slot;
	for (i = 1; i < nr_days; i++) {
		day[0] = day_list[i];
		child_slot->child = create_time_slot(day, start_hour, start_minute, end_hour, end_minute);
		child_slot = child_slot->child;
	}

#ifdef DEBUG_ME
	print_time_slot(time_slot);
#endif
	if (schedule->time_slot) {
		add_time_slot(schedule->time_slot, time_slot);
		solve(schedule->time_slot);
	} else {
		schedule->time_slot = time_slot;
	}
	return 1;
}

int compact_time_slots(time_slot_t * time_slot) {
	time_slot_t * current_slot;
	time_slot_t * child_slot;
	current_slot = time_slot;
	if (!current_slot) {
		return 0;
	}
	while (current_slot) {
		time_slot_t * child;
		child_slot = current_slot->child;
		while (child_slot) {
			child = child_slot->child;

			if (compare_time_point_by_time(&current_slot->start_time, &child_slot->start_time) == 0 && compare_time_point_by_time(&current_slot->end_time, &child_slot->end_time) == 0) {

				char start_day[14] = { '\0' };
				char end_day[14] = { '\0' };

				sprintf(start_day, "%s,%s\n", current_slot->start_time.day, child_slot->start_time.day);
				sprintf(end_day, "%s,%s\n", current_slot->end_time.day, child_slot->end_time.day);

				set_time_point_day(&current_slot->start_time, start_day);
				set_time_point_day(&current_slot->end_time, end_day);
#ifdef DEBUG_ME
				printf("  Compact time slots, Start: D:%s T:%02d:%02d, End: D:%s T:%02d:%02d\n", current_slot->start_time.day, current_slot->start_time.hour, current_slot->start_time.minute, current_slot->end_time.day, current_slot->end_time.hour,
						current_slot->end_time.minute);
#endif

				detach_time_slot(current_slot, child_slot);
				delete_time_slot(child_slot);
			}
			child_slot = child;
		}
		current_slot = current_slot->child;
	}
	return 1;
}

void enable_schedule_callback(struct tm * time, void * user_data) {
	schedule_callback * schedule_cb = (schedule_callback *) user_data;
	if (!tt_time_is_valid(time)) {
		return;
	}
	(*schedule_cb)(1);
}

void disable_schedule_callback(struct tm * time, void * user_data) {
	schedule_callback * schedule_cb = (schedule_callback *) user_data;
	if (!tt_time_is_valid(time)) {
		return;
	}
	(*schedule_cb)(0);
}

void check_schedule_state(schedule_t* schedule) {
	time_slot_t * time_slot;
	int enable;
	time_t t;
	struct tm time;
	time_point_t point = { NULL };
	enable = 0;
	if (schedule->schedule_cb) {
//		vm_get_rtc(&t); TODO get the time here
		t = 0;
		tt_localtime(t, &time);
		if (tt_time_is_valid(&time)) {
			char day[2]= {'\0'};
			day[0] = '0' + time.tm_wday;
			time_slot = schedule->time_slot;
			set_time_point(&point, day, time.tm_hour,time.tm_min);
			while (time_slot) {
				if (in_range(time_slot, &point)) {
					enable = 1;
#ifdef DEBUG_ME
					print_time_slot(time_slot);
					printf(" Match point day: %s %02d:%02d",day, time.tm_hour,time.tm_min);
#endif
					break;
				}
				time_slot = time_slot->child;
			}
		}
		schedule->schedule_cb(enable);
	}
}

void remove_scheduled_jobs(schedule_t* schedule) {
	// before we can add a schedule the old needs to be removed
	char job_name[40];
	int i = 0;
	sprintf(job_name, "%s_%d", schedule->name, i);
	printf(" Remove job: %s\n", job_name);
	while (tt_cron_remove_job(job_name)) {
		sprintf(job_name, "%s_%d", schedule->name, i);
		printf(" Remove job: %s\n", job_name);
		i++;
	}
}

int tt_scheduler_free(schedule_t* schedule) {
	delete_time_slot(schedule->time_slot);
	schedule->time_slot = NULL;
	free(schedule->name);
	schedule->name = NULL;
	return 1;
}

int tt_scheduler_remove(schedule_t* schedule) {
	schedule_holder_t * holder;
	schedule_holder_t * child;
	if (schedules) {
		if (schedules->schedule == schedule) {
			child = schedules->child;
			remove_scheduled_jobs(schedule);
			tt_scheduler_free(schedule);
			free(schedules);
			schedules = child;
			return 1;
		}
		holder = schedules;
		while (holder) {
			if (holder->child && holder->child->schedule == schedule) {
				child = holder->child;
				holder->child = child->child;
				remove_scheduled_jobs(schedule);
				tt_scheduler_free(schedule);
				free(child);
				return 1;
			}
			holder = holder->child;
		}
	}
	return 0;
}

int remove_schedulerd_jobs_by_name(char* name) {
	schedule_holder_t * holder;
	schedule_holder_t * child;
	if (schedules) {
		if (strcmp(schedules->schedule->name, name) == 0) {
			child = schedules->child;
			remove_scheduled_jobs(schedules->schedule);
			schedules = child;
			return 1;
		}
		holder = schedules;
		while (holder) {
			if (holder->child && strcmp(holder->child->schedule->name, name) == 0) {
				child = holder->child;
				holder->child = child->child;
				remove_scheduled_jobs(schedules->schedule);
				return 1;
			}
			holder = holder->child;
		}
	}
	return 0;
}

int tt_scheduler_apply(schedule_t* schedule) {
	time_slot_t * time_slot;

	compact_time_slots(schedule->time_slot);
	if (schedule->schedule_cb) {
		schedule_holder_t * holder;
		char job_name[40];
		char minute[4];
		char hour[4];
		int i;
		printf(" Applying schedule: %s",schedule->name);
		// before we can add a schedule the old needs to be removed
		remove_schedulerd_jobs_by_name(schedule->name);
		i = 0;
		time_slot = schedule->time_slot;
		while (time_slot) {
			sprintf(job_name, "%s_%d", schedule->name, i++);
			sprintf(hour, "%d", time_slot->start_time.hour);
			sprintf(minute, "%d", time_slot->start_time.minute);
			tt_cron_add_job(minute, hour, "*", "*", time_slot->start_time.day, job_name, enable_schedule_callback, &schedule->schedule_cb);
			sprintf(job_name, "%s_%d", schedule->name, i++);
			sprintf(hour, "%d", time_slot->end_time.hour);
			sprintf(minute, "%d", time_slot->end_time.minute);
			tt_cron_add_job(minute, hour, "*", "*", time_slot->end_time.day, job_name, disable_schedule_callback, &schedule->schedule_cb);
			time_slot = time_slot->child;
		}
		check_schedule_state(schedule);

		holder = malloc(sizeof(schedule_holder_t));
		holder->schedule = schedule;
		holder->child = NULL;
		if (schedules) {
			schedule_holder_t * child;
			child = schedules;
			while (child) {
				if (child->child == NULL) {
					child->child = holder;
					break;
				}
				child = child->child;
			}
		} else {
			schedules = holder;
		}
		return 1;
	}
	return 0;
}

static void delete_time_slot(time_slot_t* slot) {
	while (slot != NULL) {
		time_slot_t* child_slot;
		free(slot->start_time.day);
		free(slot->end_time.day);
		child_slot = slot->child;
		free(slot);
		slot = child_slot;
	}
}

static int validate_hour(int hour) {
	return hour >= 0 && hour < 24;
}

static int validate_minute(int minute) {
	return minute >= 0 && minute < 60;
}

static int increment_day(char * res, char *arg) {
	int n;
	char c;
	int pos = 0;
	n = 0;
	if (!strcmp(arg, "*")) {
		strcpy(res, arg);
		return 0;
	}

	while ((c = *arg++) && (c >= '0') && (c <= '9')) {
		n = (n * 10) + c - '0';
	}

	switch (c) {
	case '\0':
		if (n >= 0 && n <= 6) {
			sprintf(res, "%d", ((n + 1) % 7));
			return 1;
		}
		return 0;
		/*NOTREACHED*/
		break;
	case ',':
		if (n < 0 && n > 6) {
			return 0;
		}
		res[pos++] = '0' + ((n + 1) % 7);
		do {
			res[pos++] = ',';
			n = 0;
			while ((c = *arg++) && (c >= '0') && (c <= '9')) {
				n = (n * 10) + c - '0';
			}
			if (n < 0 || n > 6) {
				return 0;
			}
			res[pos++] = '0' + ((n + 1) % 7);
		} while (c == ',');
		res[pos++] = '\0';
		return 1;
		/*NOTREACHED*/
		break;
	case '-':
		if (n < 0 || n > 6) {
			return 0;
		}
		res[pos++] = '0' + ((n + 1) % 7);
		n = 0;
		while ((c = *arg++) && (c >= '0') && (c <= '9')) {
			n = (n * 10) + c - '0';
		}
		if (n < 0 || n > 6) {
			return 0;
		}
		res[pos++] = '-';
		res[pos++] = '0' + ((n + 1) % 7);
		res[pos++] = '\0';
		return 1;
		/*NOTREACHED*/
		break;
	default:
		break;
	}
	return 0;
}

static int split_day(char * res, char *arg) {
	int n;
	int s;
	char c;
	int pos = 0;
	n = 0;
	if (arg == NULL || res == NULL) {
		return 0;
	}

	while ((c = *arg++) && (c >= '0') && (c <= '9')) {
		n = (n * 10) + c - '0';
	}

	switch (c) {
	case '\0':
		if (n >= 0 && n <= 6) {
			res[pos++] = '0' + (n % 7);
			res[pos++] = '\0';
			return 1;
		}
		return 0;
		/*NOTREACHED*/
		break;
	case ',':
		if (n < 0 && n > 6) {
			return 0;
		}
		res[pos++] = '0' + (n % 7);
		do {
			n = 0;
			while ((c = *arg++) && (c >= '0') && (c <= '9')) {
				n = (n * 10) + c - '0';
			}
			if (n < 0 || n > 6) {
				return 0;
			}
			res[pos++] = '0' + (n % 7);
		} while (c == ',');
		res[pos++] = '\0';
		return pos - 1;
		/*NOTREACHED*/
		break;
	case '-':
		if (n < 0 || n > 6) {
			return 0;
		}
		s = n;
//		res[pos++] = '0' + (n % 7);
		n = 0;
		while ((c = *arg++) && (c >= '0') && (c <= '9')) {
			n = (n * 10) + c - '0';
		}
		if (n < 0 || n > 6) {
			return 0;
		}
		{
			int i;
			int d;
			if (n < s) {
				d = n + 7 - s;
			} else {
				d = n - s;
			}
			d++;
			for (i = 0; i < d; i++) {
				//			printf(" MOD: %d, s: %d, n: %d",(s+7-n)%7,s,n);
				res[pos++] = '0' + ((s + i) % 7);
			}
		}
		res[pos++] = '\0';
		return pos - 1;
		/*NOTREACHED*/
		break;
	default:
		break;
	}
	return 0;
}

// Compares to times
// returns 0 if the times are equal
// > 0 if time a is before time b
// < 0 if time a is after time b
static int compare_time_point_by_time(time_point_t * a, time_point_t * b) {
	return compare_time(a->hour, a->minute, b->hour, b->minute);
}

// Compares to times
// returns 0 if the times are equal
// > 0 if time a is before time b
// < 0 if time a is after time b
static int compare_time(int hour_a, int minute_a, int hour_b, int minute_b) {
	int time_a;
	int time_b;
	time_a = hour_a * 60 + minute_a;
	time_b = hour_b * 60 + minute_b;
	return time_b - time_a;

}

#ifdef DEBUG_ME
void print_schedule(schedule_t * schedule) {
	printf(" Schedule name: %s\n", schedule->name);
	print_time_slot(schedule->time_slot);
}

void print_time_slot(time_slot_t * slot) {
	while (slot) {
		printf("  Start: D:%s T:%02d:%02d, End: D:%s T:%02d:%02d\n", slot->start_time.day, slot->start_time.hour, slot->start_time.minute, slot->end_time.day, slot->end_time.hour, slot->end_time.minute);
		slot = slot->child;
	}
}

void print_point(time_point_t * point) {
	printf("  Time Point: D:%s T:%02d:%02d\n", point->day, point->hour, point->minute);
}
#endif

