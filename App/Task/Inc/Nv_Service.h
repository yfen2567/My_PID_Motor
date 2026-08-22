#ifndef APP_TASK_INC_NV_SERVICE_H_
#define APP_TASK_INC_NV_SERVICE_H_

#include "App_Cmd.h"
#include "ParamStore.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    App_Cmd_Type_t operation;
    ParamStore_Param_t params;
} NvRequest_t;

bool Nv_Service_Post(const NvRequest_t *request);
bool Nv_Service_Wait(NvRequest_t *request);
bool Nv_Service_IsBusy(void);
void Nv_Service_Complete(void);
uint32_t Nv_Service_GetQueueUsed(void);

#endif /* APP_TASK_INC_NV_SERVICE_H_ */
