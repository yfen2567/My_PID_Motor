/*
 * Cmd_Service.h
 *
 *  Created on: Jun 24, 2026
 *      Author: yuan
 */

#ifndef INC_TASK_CMD_SERVICE_H_
#define INC_TASK_CMD_SERVICE_H_

#include <stdbool.h>
#include <stdint.h>

#include "App_Cmd.h"
#include "Control.h"
#include "Uart_Protocol.h"

typedef enum
{
    CMD_SERVICE_OK = 0,
    CMD_SERVICE_BAD_ARG,
    CMD_SERVICE_QUEUE_NULL,
    CMD_SERVICE_ALLOC_FAIL,
    CMD_SERVICE_QUEUE_FULL
} Cmd_ServiceResult_t;

typedef enum
{
    CMD_MESSAGE_NOTICE = 0,
    CMD_MESSAGE_PARSE_ERROR,
    CMD_MESSAGE_CMD_RESULT,
    CMD_MESSAGE_STATUS,
    CMD_MESSAGE_FAULT_SNAPSHOT,
    CMD_MESSAGE_HELP,
    CMD_MESSAGE_STATS
} Cmd_MessageType_t;

typedef struct
{
    App_Cmd_Type_t command;
    App_Cmd_ExecResult_t result;
} Cmd_CommandResult_t;

typedef struct
{
    Motor_Status_t status;
    float kp;
    float ki;
    float kd;
    uint8_t adc_target_enabled;
    uint32_t tick_ms;
} Cmd_StatusMessage_t;

typedef struct
{
    uint8_t valid;
    FaultSnapshot_t snapshot;
} Cmd_FaultMessage_t;

typedef struct
{
    Cmd_MessageType_t type;
    union
    {
        Uart_ProtocolNotice_t notice;
        App_Cmd_ParseResult_t parse_result;
        Cmd_CommandResult_t command_result;
        Cmd_StatusMessage_t status;
        Cmd_FaultMessage_t fault;
        Cmd_StatsSnapshot_t stats;
    } payload;
} Cmd_Message_t;

Cmd_ServiceResult_t Cmd_Service_PostControlCommand(const App_Cmd_t *cmd);
bool Cmd_Service_TryGetControlCommand(App_Cmd_t *cmd_out);
bool Cmd_Service_PostNotice(Uart_ProtocolNotice_t notice);
bool Cmd_Service_PostParseError(App_Cmd_ParseResult_t result);

#endif /* INC_TASK_CMD_SERVICE_H_ */
