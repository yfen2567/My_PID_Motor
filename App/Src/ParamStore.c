																																												#include "ParamStore.h"
#include "app_config.h"
#include <stddef.h>
#include <string.h>
#include "stm32f1xx_hal.h"

#define PARAMSTORE_MAGIC 0x14332230U
#define PARAMSTORE_VERSION 1U
#define PARAMSTORE_FLASH_PAGE_ADDRESS  0x0800FC00UL
#define PARAMSTORE_FLASH_PAGE_SIZE     0x00000400UL
#define PARAMSTORE_FLASH_END_ADDRESS   0x08010000UL

extern uint8_t _sidata;
extern uint8_t _sdata;
extern uint8_t _edata;

static uint32_t ParamStore_Crc32Calc(const uint8_t *data, size_t length)
{
    uint32_t crc = 0xFFFFFFFFUL;
    size_t index;
    uint8_t bit;

    for (index = 0U; index < length; index++)
    {
        crc ^= data[index];
        for (bit = 0U; bit < 8U; bit++)
        {
            if ((crc & 1UL) != 0UL)
            {
                crc = (crc >> 1U) ^ 0xEDB88320UL;
            }
            else
            {
                crc >>= 1U;
            }
        }
    }

    return crc ^ 0xFFFFFFFFUL;
}

static uint32_t ParamStore_RecordCrc32Calc(const ParamStore_Record_t *record)
{
	return ParamStore_Crc32Calc((const uint8_t *)record,
			offsetof(ParamStore_Record_t, crc32));
}


static bool ParamStore_AreParamValid(const ParamStore_Param_t *param)
{
	if(param==NULL)
	{
		return false;
	}

	return ((param->kp>=0.0f)&&(param->kp<=APP_PID_GAINS_MAX)&&
			(param->ki>=0.0f)&&(param->ki<=APP_PID_GAINS_MAX)&&
			(param->kd>=0.0f)&&(param->kd<=APP_PID_GAINS_MAX));
}


static bool ParamStore_IsRecordValid(const ParamStore_Record_t *record)
{
	ParamStore_Param_t param;
	if((record->magic!=PARAMSTORE_MAGIC)||
		(record->version!=PARAMSTORE_VERSION)||
		(record->length!=sizeof(ParamStore_Record_t)))
	{
		return false;
	}

	if(record->crc32!=ParamStore_RecordCrc32Calc(record))
	{
		return false;
	}

	param.kp=record->kp;
	param.ki=record->ki;
	param.kd=record->kd;

	return ParamStore_AreParamValid(&param);
}

bool ParamStore_Load(ParamStore_Param_t *param)
{
	if(param==NULL)
	{
		return false;
	}

	ParamStore_Record_t record;
	memcpy(&record,(const void*)PARAMSTORE_FLASH_PAGE_ADDRESS,sizeof(record));
	if(!ParamStore_IsRecordValid(&record))
	{
		return false;
	}

	param->kp=record.kp;
	param->ki=record.ki;
	param->kd=record.kd;

	return true;
}


static bool ParamStore_IsLatoutSafe(void)
{
	uintptr_t data_load_start=(uintptr_t)&_sidata;
	uintptr_t data_size=(uintptr_t)&_edata - (uintptr_t)&_sdata;
	uintptr_t data_load_end=data_load_start+data_size;

    return ((PARAMSTORE_FLASH_PAGE_ADDRESS + PARAMSTORE_FLASH_PAGE_SIZE) ==
            PARAMSTORE_FLASH_END_ADDRESS) &&
           (data_load_end <= PARAMSTORE_FLASH_PAGE_ADDRESS);
}

static uint16_t ParamStore_GetHalfWord(const ParamStore_Record_t *record,
										size_t offset)
{
	uint8_t * byte=(uint8_t *)record;
	return (uint16_t)((uint16_t)byte[offset]|(uint16_t)byte[offset+1U]<<8U);
}

static bool ParamStore_ProgramHalfWord(uint32_t address,
        								const ParamStore_Record_t *record,
										size_t offset)
{
	return (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,
            					address,
								ParamStore_GetHalfWord(record, offset)) == HAL_OK);
}


bool ParamStore_Save(ParamStore_Param_t* param)
{
	if(param==NULL)
	{
		return false;
	}

    ParamStore_Record_t record;
    ParamStore_Record_t verify_record;
    FLASH_EraseInitTypeDef erase = {0};
    uint32_t page_error = 0U;
    size_t offset;
    bool success = true;

    if(!ParamStore_AreParamValid(param)||!ParamStore_IsLatoutSafe())
    {
    	return false;
    }

    record.magic=PARAMSTORE_MAGIC;
	record.version=PARAMSTORE_VERSION;
	record.length=sizeof(ParamStore_Record_t);
	record.kp=param->kp;
	record.ki=param->ki;
	record.kd=param->kd;
	record.crc32=ParamStore_RecordCrc32Calc(&record);

	erase.TypeErase=FLASH_TYPEERASE_PAGES;
	erase.PageAddress=PARAMSTORE_FLASH_PAGE_ADDRESS;
	erase.NbPages=1U;

	if(HAL_FLASH_Unlock()!=HAL_OK)
	{
		return false;
	}

	__HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPERR);

	if(HAL_FLASHEx_Erase(&erase, &page_error)!=HAL_OK)
	{
		return false;
	}

	for(offset=sizeof(record.magic);success&&offset<sizeof(record);offset+=2U)
	{
		success=ParamStore_ProgramHalfWord(PARAMSTORE_FLASH_PAGE_ADDRESS+(uint32_t)offset, &record, offset);
	}

	for(offset=0U;success&&offset<sizeof(record.magic);offset+=2U)
	{
		success=ParamStore_ProgramHalfWord(PARAMSTORE_FLASH_PAGE_ADDRESS+(uint32_t)offset, &record, offset);
	}

	if(HAL_FLASH_Lock()!=HAL_OK)
	{
		return false;
	}

    if (!success)
    {
        return false;
    }

    memcpy(&verify_record,(const void*)PARAMSTORE_FLASH_PAGE_ADDRESS,sizeof(ParamStore_Record_t));

    return ParamStore_IsRecordValid(&verify_record)&&
    		memcmp(&record,&verify_record, sizeof(ParamStore_Record_t))==0;
}

bool ParamStore_Reset(ParamStore_Param_t* params)
{
	if(params==NULL)
	{
		return false;
	}
	params->kp=APP_PID_DEFAULT_KP;
	params->ki=APP_PID_DEFAULT_KI;
	params->kd=APP_PID_DEFAULT_KD;

	return ParamStore_Save(params);
}
