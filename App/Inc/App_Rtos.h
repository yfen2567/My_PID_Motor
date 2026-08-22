/*
 * App_Rtos.h
 *
 *  Created on: Jun 23, 2026
 *      Author: yuan
 */

#ifndef INC_APP_RTOS_H_
#define INC_APP_RTOS_H_

#include "cmsis_os.h"

extern osMessageQueueId_t ControlCmdQueueHandle;
extern osMessageQueueId_t CommTxQueueHandle;
extern osSemaphoreId_t s_tx_done_SemHandle;
extern osMessageQueueId_t NvRequestQueueHandle;
#endif /* INC_APP_RTOS_H_ */
