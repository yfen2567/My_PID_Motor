/*
 * LogTask.h
 *
 *  Created on: Jun 24, 2026
 *      Author: yuan
 */

#ifndef TASK_INC_LOGTASK_H_
#define TASK_INC_LOGTASK_H_

#include "stdbool.h"

void StartLogTask(void *argument);
bool LogTask_PostPeriodicStatus(void);
bool LogTask_PostFaultSnapshot(void);

#endif /* TASK_INC_LOGTASK_H_ */
