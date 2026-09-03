/*
 * Control_Task.h
 *
 *  Created on: Jun 24, 2026
 *      Author: yuan
 */

#ifndef TASK_INC_CONTROL_TASK_H_
#define TASK_INC_CONTROL_TASK_H_

typedef struct
{
	uint32_t period_us;
	uint32_t execution_us;
	uint32_t control_tick_execution_us ;
	uint16_t timeout_count;
}Control_TimingStats_t;

void Control_Timing_GetStats(Control_TimingStats_t *out);

#endif /* TASK_INC_CONTROL_TASK_H_ */
