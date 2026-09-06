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
	uint32_t sample_count;
	uint32_t period_us;
	uint32_t period_min_us;
	uint32_t period_max_us;
	uint32_t max_abs_jitter_us;
	uint32_t execution_us;
	uint32_t execution_min_us;
	uint32_t execution_max_us;
	uint32_t control_tick_execution_us ;
	uint32_t control_tick_min_us;
	uint32_t control_tick_max_us;
	uint16_t timeout_count;
}Control_TimingStats_t;

void Control_Timing_GetStats(Control_TimingStats_t *out);

#endif /* TASK_INC_CONTROL_TASK_H_ */
