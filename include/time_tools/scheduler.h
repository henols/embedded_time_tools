/*
 *
 * © 2016 Henrik Olsson, henols@gmail.com
 *
 * scheduler.h
 *
 *  Created on: 25 nov. 2017
 *      Author: Henrik
 */

#ifndef SCHEDULER_H_
#define SCHEDULER_H_


typedef void (*schedule_callback) (int value);

typedef struct time_point {
	char * day;
	int day_num;
	int hour;
	int minute;
} time_point_t;


typedef struct time_slot {
	time_point_t start_time;
	time_point_t end_time;
	struct time_slot * child;
} time_slot_t;

typedef struct schedule {
	char * name;
	time_slot_t * time_slot;
	schedule_callback schedule_cb;
} schedule_t;

/**
 * Initiates the scheduler
 */
void tt_scheduler_init(void);

/**
 * Destroys the scheduler and removes all applied schedules
 */
void tt_scheduler_deinit(void);

/*
 * Creates a schedule instance
 */
void tt_scheduler_init_schedule(	schedule_t * schedule, char * name, schedule_callback schedule_cb);

/**
 * Solves a time slot and adds it to the schedule
 */
int tt_scheduler_solve(schedule_t* schedule, char* day, int start_hour, int start_minute, int end_hour, int end_minute);

/**
 * Applies the schedule for execution
 */
int tt_scheduler_apply(schedule_t* schedule);

/**
 * Removes an applied schedule
 */
int tt_scheduler_remove(schedule_t* schedule);

/**
 * Free's all nested time slots in the schedule
 */
int tt_scheduler_free(schedule_t* schedule);

#endif /* SCHEDULER_H_ */
