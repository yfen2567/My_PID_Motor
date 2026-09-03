#ifndef APP_INC_COMM_SERVICE_H_
#define APP_INC_COMM_SERVICE_H_

#include "App_Cmd.h"
#include "Control.h"
#include "Uart_Protocol.h"
#include  "Control_Task.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    COMM_MESSAGE_NOTICE = 0,
    COMM_MESSAGE_PARSE_ERROR,
    COMM_MESSAGE_COMMAND_RESULT,
    COMM_MESSAGE_STATUS,
    COMM_MESSAGE_FAULT_SNAPSHOT,
    COMM_MESSAGE_HELP,
    COMM_MESSAGE_STATS
} Comm_MessageType_t;

typedef struct
{
    App_Cmd_Type_t command;
    App_Cmd_ExecResult_t result;
} Comm_CommandResult_t;

typedef struct
{
    Motor_Status_t status;
    float kp;
    float ki;
    float kd;
    uint8_t adc_target_enabled;
    uint32_t tick_ms;
} Comm_StatusMessage_t;

typedef struct
{
    uint8_t valid;
    FaultSnapshot_t snapshot;
} Comm_FaultMessage_t;

typedef struct
{
    Comm_MessageType_t type;
    union
    {
        Uart_ProtocolNotice_t notice;
        App_Cmd_ParseResult_t parse_result;
        Comm_CommandResult_t command_result;
        Comm_StatusMessage_t status;
        Comm_FaultMessage_t fault;
        Comm_StatsSnapshot_t stats;
        Control_TimingStats_t stats2;
    } payload;
} Comm_Message_t;

bool Comm_Service_PostNotice(Uart_ProtocolNotice_t notice);
bool Comm_Service_PostParseError(App_Cmd_ParseResult_t result);
bool Comm_Service_PostStatus(const Motor_Status_t *status,
                             float kp, float ki, float kd,
                             uint8_t adc_target_enabled,
                             uint32_t tick_ms);
bool Comm_Service_PostFaultSnapshot(uint8_t valid,
                                    const FaultSnapshot_t *snapshot);
bool Comm_Service_PostHelp(void);

bool Comm_Service_PostCommandResult(App_Cmd_Type_t command,
                                    App_Cmd_ExecResult_t result);
bool Comm_Service_ProcessTx(void);


uint32_t Comm_Service_GetDroppedCount(void);

uint32_t Comm_Service_GetTxErrorCount(void);

uint32_t Comm_Service_GetQueueUsed(void);

uint32_t Comm_Service_GetQueueUsedMax(void);

uint32_t Comm_Service_GetQueueCapacity(void);
bool Comm_Service_PostStats(void);
#endif /* APP_INC_COMM_SERVICE_H_ */
