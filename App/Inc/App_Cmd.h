/*
 * App_Cmd.h
 *
 *  Created on: Jun 23, 2026
 *      Author: yuan
 */

#ifndef INC_App_Cmd_H_
#define INC_App_Cmd_H_

#include <stdint.h>
#include <stdbool.h>
typedef enum{

	    APP_CMD_NONE = 0,

	    APP_CMD_RUN,
	    APP_CMD_STOP,
	    APP_CMD_SET_TARGET,

	    APP_CMD_SET_TARGET_ADC,
	    APP_CMD_SET_TARGET_UART,

	    APP_CMD_SET_KP,
	    APP_CMD_SET_KI,
	    APP_CMD_SET_KD,

	    APP_CMD_RESET,

	    APP_CMD_STATUS,
	    APP_CMD_HELP,
	    APP_CMD_GET_FAULT,
		APP_CMD_COMM_STATS,

		APP_CMD_LOAD_PARAMS,
		APP_CMD_SAVE_PARAMS,
		APP_CMD_RESET_PARAMS,

		APP_CMD_APPLY_STORED_PARAMS,
}App_Cmd_Type_t;

typedef struct
{
    int32_t value;
    App_Cmd_Type_t type;
    float fvalue;
    float kp;
    float ki;
    float kd;
    App_Cmd_Type_t origin_type;
} App_Cmd_t;

typedef enum
{
    APP_CMD_PARSE_OK = 0,
    APP_CMD_PARSE_EMPTY,
    APP_CMD_PARSE_INVALID,
    APP_CMD_PARSE_OUT_OF_RANGE,
    APP_CMD_PARSE_BAD_INT,
    APP_CMD_PARSE_BAD_FLOAT,
    APP_CMD_PARSE_TOO_LONG
} App_Cmd_ParseResult_t;

typedef enum
{
    APP_CMD_EXEC_OK = 0,
    APP_CMD_EXEC_BAD_ARG,
    APP_CMD_EXEC_NOT_IDLE,
    APP_CMD_EXEC_BUSY,
    APP_CMD_EXEC_INVALID_STATE,
    APP_CMD_EXEC_OUT_OF_RANGE,
    APP_CMD_EXEC_QUEUE_FULL,
    APP_CMD_EXEC_POOL_EMPTY,
    APP_CMD_EXEC_STORAGE_READ_FAILED,
    APP_CMD_EXEC_STORAGE_WRITE_FAILED,
    APP_CMD_EXEC_UNSUPPORTED
} App_Cmd_ExecResult_t;

App_Cmd_ParseResult_t App_Cmd_Parse(const char *line, App_Cmd_t *cmd);
uint8_t App_Cmd_IsParamStoreCommand(App_Cmd_Type_t type);

#endif /* INC_APPCOMMAND_H_ */
