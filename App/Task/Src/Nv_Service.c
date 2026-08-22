#include "Nv_Service.h"

#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "task.h"
#include "App_Rtos.h"


static uint8_t s_busy;

void Nv_Service_Complete(void)
{
    taskENTER_CRITICAL();
    s_busy = 0U;
    taskEXIT_CRITICAL();
}

bool Nv_Service_Post(const NvRequest_t *request)
{
    uint8_t accepted = 0U;

    if ((request == NULL) || (NvRequestQueueHandle == NULL)) { return false; }
    taskENTER_CRITICAL();
    if (s_busy == 0U)
    {
        s_busy = 1U;
        accepted = 1U;
    }
    taskEXIT_CRITICAL();
    if (accepted == 0U) { return false; }

    if (osMessageQueuePut(NvRequestQueueHandle, request, 0U, 0U) != osOK)
    {
        Nv_Service_Complete();
        return false;
    }
    return true;
}

bool Nv_Service_Wait(NvRequest_t *request)
{
    return ((request != NULL) && (NvRequestQueueHandle != NULL) &&
            (osMessageQueueGet(NvRequestQueueHandle, request, NULL, osWaitForever) == osOK));
}

bool Nv_Service_IsBusy(void)
{
    uint8_t busy;
    taskENTER_CRITICAL();
    busy = s_busy;
    taskEXIT_CRITICAL();
    return (busy != 0U);
}



uint32_t Nv_Service_GetQueueUsed(void)
{
    return (NvRequestQueueHandle != NULL) ? osMessageQueueGetCount(NvRequestQueueHandle) : 0U;
}
