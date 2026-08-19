#include "App_Cmd.h"
#include "app_config.h"
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static App_Cmd_ParseResult_t App_Cmd_ParseInt(const char *text, int32_t *value)
{
    char *end = NULL;
    long result = 0;

    if ((text == NULL) || (value == NULL))
    {
        return APP_CMD_PARSE_BAD_INT;
    }

    errno = 0;
    result = strtol(text, &end, 10);
    if ((end == text) ||
    		(*end != '\0')||
    		(errno==ERANGE)||
    		((result < INT32_MIN) ||
    		(result > INT32_MAX)))
    {
        return APP_CMD_PARSE_BAD_INT;
    }

    *value = (int32_t)result;
    return APP_CMD_PARSE_OK;
}

static uint8_t App_Cmd_ParseFloat(const char *text, float *value)
{
    char *end = NULL;
    float result = 0.0f;

    if ((text == NULL) || (value == NULL))
    {
        return 0U;
    }

    result = strtof(text, &end);
    if ((end == text) || (*end != '\0'))
    {
        return 0U;
    }

    *value = result;
    return 1U;
}

static uint8_t App_Cmd_CheckPidGains(float gains)
{
    return ((gains >= 0.0f) && (gains <= 100.0f)) ? 1U : 0U;
}

App_Cmd_ParseResult_t App_Cmd_Parse(const char *line, App_Cmd_t *cmd)
{
    int32_t target = 0;
    float fvalue = 0.0f;

    if ((line == NULL) || (cmd == NULL))
    {
        return APP_CMD_PARSE_INVALID;
    }

    cmd->type = APP_CMD_NONE;
    cmd->value = 0;
    cmd->fvalue = 0.0f;

    if (strcmp(line, "run=1") == 0)
    {
        cmd->type = APP_CMD_RUN;
        return APP_CMD_PARSE_OK;
    }

    if ((strcmp(line, "stop") == 0) || (strcmp(line, "run=0") == 0))
    {
        cmd->type = APP_CMD_STOP;
        return APP_CMD_PARSE_OK;
    }

    if (strncmp(line, "t=", 2) == 0)
    {
        if (App_Cmd_ParseInt(line + 2, &target) != APP_CMD_PARSE_OK)
        {
            return APP_CMD_PARSE_BAD_INT;
        }

        if ((target < -APP_TARGET_SPEED_MAX) || (target > APP_TARGET_SPEED_MAX))
        {
            return false;
        }

        cmd->type = APP_CMD_SET_TARGET;
        cmd->value = target;
        return APP_CMD_PARSE_OK;
    }

    if (strncmp(line, "kp=", 3) == 0)
    {
        if ((App_Cmd_ParseFloat(line + 3, &fvalue) == 0U) ||
            (App_Cmd_CheckPidGains(fvalue) == 0U))
        {
            return false;
        }

        cmd->type = APP_CMD_SET_KP;
        cmd->fvalue = fvalue;
        return APP_CMD_PARSE_OK;
    }

    if (strncmp(line, "ki=", 3) == 0)
    {
        if ((App_Cmd_ParseFloat(line + 3, &fvalue) == 0U) ||
            (App_Cmd_CheckPidGains(fvalue) == 0U))
        {
            return false;
        }

        cmd->type = APP_CMD_SET_KI;
        cmd->fvalue = fvalue;
        return APP_CMD_PARSE_OK;
    }

    if (strncmp(line, "kd=", 3) == 0)
    {
        if ((App_Cmd_ParseFloat(line + 3, &fvalue) == 0U) ||
            (App_Cmd_CheckPidGains(fvalue) == 0U))
        {
            return false;
        }

        cmd->type = APP_CMD_SET_KD;
        cmd->fvalue = fvalue;
        return APP_CMD_PARSE_OK;
    }

    if (strcmp(line, "rst") == 0)
    {
        cmd->type = APP_CMD_RESET;
        return APP_CMD_PARSE_OK;
    }

    if (strcmp(line, "set target adc") == 0)
    {
        cmd->type = APP_CMD_SET_TARGET_ADC;
        return APP_CMD_PARSE_OK;
    }

    if (strcmp(line, "set target uart") == 0)
    {
        cmd->type = APP_CMD_SET_TARGET_UART;
        return APP_CMD_PARSE_OK;
    }

    if (strcmp(line, "status") == 0)
    {
        cmd->type = APP_CMD_STATUS;
        return APP_CMD_PARSE_OK;
    }

    if (strcmp(line, "help") == 0)
    {
        cmd->type = APP_CMD_HELP;
        return APP_CMD_PARSE_OK;
    }

    if (strcmp(line, "get fault") == 0)
    {
        cmd->type = APP_CMD_GET_FAULT;
        return APP_CMD_PARSE_OK;
    }

    if(strcmp(line, "load params") == 0)
    {
    	cmd->type = APP_CMD_LOAD_PARAMS;
    	return APP_CMD_PARSE_OK;
    }

    if(strcmp(line, "save params") == 0)
    {
    	cmd->type = APP_CMD_SAVE_PARAMS;
    	return APP_CMD_PARSE_OK;
    }

    if(strcmp(line, "load params") == 0)
    {
    	cmd->type = APP_CMD_RESET_PARAMS;
    	return APP_CMD_PARSE_OK;
    }

    return false;
}
