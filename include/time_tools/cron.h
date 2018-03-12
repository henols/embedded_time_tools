/*
 *
 * © 2016 Henrik Olsson, henols@gmail.com
 *
 * cron.h
 *
 *  Created on: 20 mars 2017
 *      Author: Henrik
 */

#ifndef CRON_H_
#define CRON_H_

#include <time.h>

typedef void (*CRON_JOB) (struct tm * time, void * user_data);

void tt_cron_refresh(void);
void tt_cron_add_job(char *mn, char *hr, char *day, char *mon, char *wkd, char *cmd, CRON_JOB job,void * user_data);
int tt_cron_remove_job(char *cmd);

#endif /* CRON_H_ */
