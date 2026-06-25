/*
 * Cmd_Service.h
 *
 *  Created on: Jun 24, 2026
 *      Author: yuan
 */

#ifndef INC_TASK_CMD_SERVICE_H_
#define INC_TASK_CMD_SERVICE_H_
#include "stdbool.h"
#include "App_Cmd.h"
typedef enum
{
    CMD_SERVICE_OK = 0,
	CMD_SERVICE_BAD_ARG,
	CMD_SERVICE_QUEUE_NULL,
	CMD_SERVICE_ALLOC_FAIL,
	CMD_SERVICE_QUEUE_FULL
} Cmd_ServiceResult_t;

Cmd_ServiceResult_t Cmd_Service_PostControlCommand(const App_Cmd_t *cmd);
bool Cmd_Service_TryGetControlCommand(App_Cmd_t *cmd_out);
#endif /* INC_TASK_CMD_SERVICE_H_ */
