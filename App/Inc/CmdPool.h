/*
 * CmdPool.h
 *
 *  Created on: Aug 21, 2026
 *      Author: yuan
 */

#ifndef INC_CMDPOOL_H_
#define INC_CMDPOOL_H_

#include "App_Cmd.h"

#include <stdint.h>

#define CMD_POOL_CAPACITY  16U

void CmdPool_Init(void);
App_Cmd_t *CmdPool_Alloc(void);
void CmdPool_Free(App_Cmd_t *cmd);

#endif /* INC_CMDPOOL_H_ */
