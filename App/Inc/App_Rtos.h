/*
 * App_Rtos.h
 *
 *  Created on: Jun 23, 2026
 *      Author: yuan
 */

#ifndef INC_APP_RTOS_H_
#define INC_APP_RTOS_H_

#include "cmsis_os.h"

extern osMessageQueueId_t CmdQueueHandle;
extern osMutexId_t uartTxMutexHandle;
#endif /* INC_APP_RTOS_H_ */
