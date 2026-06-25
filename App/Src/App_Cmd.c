#include "App_Cmd.h"
#include "app_config.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static uint8_t App_Cmd_ParseInt(const char *text, int32_t *value)
{
    char *end = NULL;
    long result = 0;

    if ((text == NULL) || (value == NULL))
    {
        return 0U;
    }

    result = strtol(text, &end, 10);
    if ((end == text) || (*end != '\0'))
    {
        return 0U;
    }

    *value = (int32_t)result;
    return 1U;
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

bool App_Cmd_Parse(const char *line, App_Cmd_t *cmd)
{
    int32_t target = 0;
    float fvalue = 0.0f;

    if ((line == NULL) || (cmd == NULL))
    {
        return false;
    }

    cmd->type = APP_CMD_NONE;
    cmd->value = 0;
    cmd->fvalue = 0.0f;

    if (strcmp(line, "run=1") == 0)
    {
        cmd->type = APP_CMD_RUN;
        return true;
    }

    if ((strcmp(line, "stop") == 0) || (strcmp(line, "run=0") == 0))
    {
        cmd->type = APP_CMD_STOP;
        return true;
    }

    if (strncmp(line, "t=", 2) == 0)
    {
        if (App_Cmd_ParseInt(line + 2, &target) == 0U)
        {
            return false;
        }

        if ((target < -APP_TARGET_SPEED_MAX) || (target > APP_TARGET_SPEED_MAX))
        {
            return false;
        }

        cmd->type = APP_CMD_SET_TARGET;
        cmd->value = target;
        return true;
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
        return true;
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
        return true;
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
        return true;
    }

    if (strcmp(line, "rst") == 0)
    {
        cmd->type = APP_CMD_RESET;
        return true;
    }

    if (strcmp(line, "set target adc") == 0)
    {
        cmd->type = APP_CMD_SET_TARGET_ADC;
        return true;
    }

    if (strcmp(line, "set target uart") == 0)
    {
        cmd->type = APP_CMD_SET_TARGET_UART;
        return true;
    }

    if (strcmp(line, "status") == 0)
    {
        cmd->type = APP_CMD_STATUS;
        return true;
    }

    if (strcmp(line, "help") == 0)
    {
        cmd->type = APP_CMD_HELP;
        return true;
    }

    if (strcmp(line, "get fault") == 0)
    {
        cmd->type = APP_CMD_GET_FAULT;
        return true;
    }

    return false;
}
