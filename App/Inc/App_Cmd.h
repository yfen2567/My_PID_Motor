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
	    APP_CMD_GET_FAULT

}App_Cmd_Type_t;

typedef struct{
	int32_t value;
	App_Cmd_Type_t type;
	float fvalue;
}App_Cmd_t;

typedef enum
{
    APP_CMD_PARSE_OK = 0,
    APP_CMD_PARSE_EMPTY,
    APP_CMD_PARSE_INVALID,
    APP_CMD_PARSE_OUT_OF_RANGE
} App_Cmd_ParseResult_t;

bool App_Cmd_Parse(const char *line, App_Cmd_t *cmd);

#endif /* INC_APPCOMMAND_H_ */
