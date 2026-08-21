#include "CommTxTask.h"
#include "Comm_Service.h"

void StartCommTxTask(void *argument)
{
    (void)argument;
    for (;;)
    {
        (void)Comm_Service_ProcessTx();
    }
}
