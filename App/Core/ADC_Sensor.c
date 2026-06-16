#include "ADC_Sensor.h"
#include "adc.h"
#include "app_config.h"
static uint16_t s_adc_raw=0;
static volatile uint16_t s_adc_dma_buf[1]={0};
static float s_adc_filtered=0;
static uint8_t s_adc_filtered_ready=0;
void ADC_Init(){
	if(HAL_ADCEx_Calibration_Start(&hadc1)!=HAL_OK){
		Error_Handler();
	}
	if(HAL_ADC_Start_DMA(&hadc1, (uint32_t*)s_adc_dma_buf, 1U)!=HAL_OK){
		Error_Handler();
	}
	if(hadc1.DMA_Handle!=NULL){
		__HAL_DMA_DISABLE_IT(hadc1.DMA_Handle,DMA_IT_TC);
		__HAL_DMA_DISABLE_IT(hadc1.DMA_Handle,DMA_IT_HT);
	}
	s_adc_raw=0;
	s_adc_dma_buf[0]=0;
	s_adc_filtered=0;
	s_adc_filtered_ready = 0;
}

void ADC_Update(){
	s_adc_raw=s_adc_dma_buf[0];
	if(s_adc_filtered_ready==0){
			s_adc_filtered=s_adc_raw;
			s_adc_filtered_ready=1;
		}
		else{
		s_adc_filtered=s_adc_filtered+APP_FILTERED_ALPHA* ((float)s_adc_raw - s_adc_filtered);
		}

}

uint16_t ADC_Get_Raw(){
	return s_adc_raw;
}

float ADC_Get_Filtered(){
	return s_adc_filtered;
}

int32_t ADC_MapTargetSpeed(){
	float filtered = s_adc_filtered;
	 if (filtered < 0.0f)
	    {
	        filtered = 0.0f;
	    }
	 else if(filtered>APP_ADC_FILTERED_ABS_MAX){
		 filtered=APP_ADC_FILTERED_ABS_MAX;
	 }
	 return (int32_t)((filtered * (float)APP_TARGET_SPEED_MAX) /
	                      (float)APP_ADC_FILTERED_ABS_MAX);
}
