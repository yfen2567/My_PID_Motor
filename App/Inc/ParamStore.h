/*
 * ParamStore.h
 *
 *  Created on: Jun 30, 2026
 *      Author: yuan
 */

#ifndef INC_PARAMSTORE_H_
#define INC_PARAMSTORE_H_
#include <stdint.h>
#include <stdbool.h>

typedef struct
{
	uint32_t magic;
	uint16_t version;
	uint16_t length;
	float kp;
	float ki;
	float kd;
	uint32_t crc32;

} ParamStore_Record_t;

typedef struct
{
	float kp;
	float ki;
	float kd;

} ParamStore_Param_t;

bool ParamStore_Load(ParamStore_Param_t *param);
bool ParamStore_Save(const ParamStore_Param_t *params);
bool ParamStore_Reset(ParamStore_Param_t* params);
void ParamStore_GetDefaults(ParamStore_Param_t *params);

#endif /* INC_PARAMSTORE_H_ */
