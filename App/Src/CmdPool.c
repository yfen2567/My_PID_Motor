#include "CmdPool.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "App_Cmd.h"
#include "FreeRTOS.h"
#include "task.h"

static App_Cmd_t s_blocks[CMD_POOL_CAPACITY];
static uint8_t s_in_use[CMD_POOL_CAPACITY];
static uint32_t s_alloc_fail_count;
static uint32_t s_in_use_count;
static uint32_t s_in_use_max;

void CmdPool_Init(void)
{
    memset(s_blocks, 0, sizeof(s_blocks));
    memset(s_in_use, 0, sizeof(s_in_use));
    s_alloc_fail_count = 0U;
    s_in_use_count = 0U;
    s_in_use_max = 0U;
}

App_Cmd_t *CmdPool_Alloc(void)
{
    App_Cmd_t *result = NULL;
    uint32_t index;

    taskENTER_CRITICAL();
    for (index = 0U; index < CMD_POOL_CAPACITY; index++)
    {
        if (s_in_use[index] == 0U)
        {
            s_in_use[index] = 1U;
            s_in_use_count++;
            if (s_in_use_count > s_in_use_max) { s_in_use_max = s_in_use_count; }
            result = &s_blocks[index];
            break;
        }
    }
    if (result == NULL) { s_alloc_fail_count++; }
    taskEXIT_CRITICAL();

    if (result != NULL) { memset(result, 0, sizeof(*result)); }
    return result;
}

void CmdPool_Free(App_Cmd_t *cmd)
{
    uintptr_t address = (uintptr_t)cmd;
    const uintptr_t pool_start = (uintptr_t)&s_blocks[0];
    const uintptr_t pool_end = (uintptr_t)&s_blocks[CMD_POOL_CAPACITY];
    uint32_t index;

    if ((address < pool_start) || (address >= pool_end) ||
        (((address - pool_start) % sizeof(App_Cmd_t)) != 0U)) { return; }
    index = (uint32_t)((address - pool_start) / sizeof(App_Cmd_t));
    taskENTER_CRITICAL();
    if (s_in_use[index] != 0U)
    {
        s_in_use[index] = 0U;
        if (s_in_use_count > 0U) { s_in_use_count--; }
    }
    taskEXIT_CRITICAL();
}

uint32_t CmdPool_GetAllocFailCount(void)
{
    return s_alloc_fail_count;
}

uint32_t CmdPool_GetInUse(void)
{
    return s_in_use_count;
}

uint32_t CmdPool_GetInUseMax(void)
{
    return s_in_use_max;
}

uint32_t CmdPool_GetCapacity(void)
{
    return CMD_POOL_CAPACITY;
}
