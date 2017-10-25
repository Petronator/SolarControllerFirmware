/**
	* This file contains all functions related to analog measurement, including operation of the ADC and DMA
	*/
	
#include "main.h"
#include "stm32l0xx_hal.h"
#include "analog.h"



	uint16_t ADCReadings[7];					//Stored values of latest ADC readings, from DMA
	ADC_HandleTypeDef hadc;						//handle for ADC
	DMA_HandleTypeDef hdma_adc;				//handle for DMA
	int32_t AN_VDDA;									//Voltage of the analog supply (VCC)
	int32_t BatteryVoltage, LoadVoltage, LoadCurrent, SolarCurrent;
	uint16_t vrefint_cal;							//Calibrated measurement of AN supply at 3.0 V
	
	volatile int32_t battAcumCharge_mC = 0;	
	volatile uint32_t lastAcumTick;
	
	
	
		/* ADC init function */
	static void MX_ADC_Init(void)
	{
		

  ADC_ChannelConfTypeDef sConfig;
	ADC_OversamplingTypeDef oversampConfig;

    /**Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion) 
    */
	oversampConfig.Ratio = ADC_OVERSAMPLING_RATIO_32;
	oversampConfig.RightBitShift = ADC_RIGHTBITSHIFT_5;
	
	
  hadc.Instance = ADC1;
  hadc.Init.OversamplingMode = ENABLE;
	hadc.Init.Oversample = oversampConfig;
  hadc.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
  hadc.Init.Resolution = ADC_RESOLUTION_12B;
  hadc.Init.SamplingTime = ADC_SAMPLETIME_12CYCLES_5;
  hadc.Init.ScanConvMode = ADC_SCAN_DIRECTION_FORWARD;
  hadc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc.Init.ContinuousConvMode = ENABLE;
  hadc.Init.DiscontinuousConvMode = DISABLE;
  hadc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc.Init.DMAContinuousRequests = ENABLE;
  hadc.Init.EOCSelection = ADC_EOC_SEQ_CONV;
  hadc.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
  hadc.Init.LowPowerAutoWait = DISABLE;
  hadc.Init.LowPowerFrequencyMode = DISABLE;
  hadc.Init.LowPowerAutoPowerOff = DISABLE;
  if (HAL_ADC_Init(&hadc) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }
	ADC1->CFGR1 |= ADC_CFGR1_DMAEN | ADC_CFGR1_DMACFG;
	
	

    /**Configure for the selected ADC regular channel to be converted. 
    */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = ADC_RANK_CHANNEL_NUMBER;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Configure for the selected ADC regular channel to be converted. 
    */
  sConfig.Channel = ADC_CHANNEL_1;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Configure for the selected ADC regular channel to be converted. 
    */
  sConfig.Channel = ADC_CHANNEL_4;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Configure for the selected ADC regular channel to be converted. 
    */
  sConfig.Channel = ADC_CHANNEL_5;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Configure for the selected ADC regular channel to be converted. 
    */
  sConfig.Channel = ADC_CHANNEL_6;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Configure for the selected ADC regular channel to be converted. 
    */
  sConfig.Channel = ADC_CHANNEL_TEMPSENSOR;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Configure for the selected ADC regular channel to be converted. 
    */
  sConfig.Channel = ADC_CHANNEL_VREFINT;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}
	
/** 
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void) 
{
  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel1_IRQn interrupt configuration */
  //HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
  //HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);

}
	
void AN_Init(void)
{
	
	__HAL_RCC_ADC1_CLK_ENABLE();	
			//before starting the ADC, perform a calibration
	if ((ADC1->CR & ADC_CR_ADEN) != 0) /* (1) */
	{
		ADC1->CR &= (uint32_t)(~ADC_CR_ADEN); /* (2) */
	}
	ADC1->CR |= ADC_CR_ADCAL; /* (3) */
	while ((ADC1->ISR & ADC_ISR_EOCAL) == 0) /* (4) */
	{
		/* For robust implementation, add here time-out management */
	}
	ADC1->ISR |= ADC_ISR_EOCAL; /* (5) */

	MX_ADC_Init();

	
	MX_DMA_Init();
	

	//HAL_ADC_Start(&hadc);
	HAL_ADC_Start_DMA(&hadc, (uint32_t*) ADCReadings, 7); //Starts DMA collection of ADC data
	
	vrefint_cal= *((uint16_t*)VREFINT_CAL_ADDR);		//Fetch the int calibration value
	
	lastAcumTick = HAL_GetTick();
}

int32_t AN_getBattVoltage(void)
{
	//uint32_t t1,t2,t3,t4;
	
	//Calculate full-scale voltage
	
	AN_VDDA = (3000 * (uint32_t)vrefint_cal) / ADCReadings[AN_num_Intref];
	
	//t1 = ADC_IS_ENABLE(&hadc);
	//t2 = ADC_IS_CONVERSION_ONGOING_REGULAR(&hadc);
	//at3 = HAL_ADC_GetValue(&hadc);
	//HAL_DMA_GetState();
	
	BatteryVoltage = (ADCReadings[AN_num_Vbatt]*2 * AN_VDDA) / FULL_SCALE;
	//Assume .5 ohm source resistance in this estimation
	BatteryVoltage -= AN_getBattCurrent()/2;
	
	return BatteryVoltage;
}

int32_t AN_getLoadVoltage(void)
{
	//Calculate full-scale voltage
	
	AN_VDDA = (3000 * (uint32_t)vrefint_cal) / ADCReadings[AN_num_Intref];
	
	LoadVoltage = (ADCReadings[AN_num_Vload]*2 * AN_VDDA) / FULL_SCALE;
	return LoadVoltage;
}
	
int32_t AN_getBattCurrent(void)
{
	//Positive is charging;
	int32_t t1, t2;
	AN_VDDA = (3000 * (uint32_t)vrefint_cal) / ADCReadings[AN_num_Intref];
	
	t1 = (ADCReadings[AN_num_Isolar] * AN_VDDA) / FULL_SCALE;
	SolarCurrent = (t1 * 1000)/VOLTTOAMP_MOHM_S;
	t2 = (ADCReadings[AN_num_Iload] * AN_VDDA) / FULL_SCALE;
	LoadCurrent = (t2 * 1000)/VOLTTOAMP_MOHM_L;
	return (SolarCurrent - LoadCurrent);
}

int32_t AN_getSolarPower(void)
{
	uint32_t t1, t2;
	AN_VDDA = (3000 * (uint32_t)vrefint_cal) / ADCReadings[AN_num_Intref];
	
	t1 = (ADCReadings[AN_num_Vbatt]*2 * AN_VDDA) / FULL_SCALE;
	t2 = ((ADCReadings[AN_num_Isolar]) * AN_VDDA) / FULL_SCALE;
	SolarCurrent = (t2 * 1000)/VOLTTOAMP_MOHM_S;
	
	return (t1*SolarCurrent)/1000;
}

int32_t AN_getLoadPower(void)
{
	int32_t t1, t2;
	AN_VDDA = (3000 * (uint32_t)vrefint_cal) / ADCReadings[AN_num_Intref];
	
	t1 = (ADCReadings[AN_num_Vbatt]*2 * AN_VDDA) / FULL_SCALE;
	t2 = (ADCReadings[AN_num_Iload] * AN_VDDA) / FULL_SCALE;
	t2 = (t2 * 1000)/VOLTTOAMP_MOHM_L;
	
	return (t1*t2)/1000;
}

int32_t AN_getLoadCurrent(void)
{
	uint32_t t1, t2;
	//we might not have initialized yet. Then just return 0
	if (ADCReadings[AN_num_Intref] == 0) return 0;
	
	AN_VDDA = (3000 * (uint32_t)vrefint_cal) / ADCReadings[AN_num_Intref];
	

	t2 = (ADCReadings[AN_num_Iload] * AN_VDDA) / FULL_SCALE;
	LoadCurrent = (t2 * 1000)/VOLTTOAMP_MOHM_L;
	return LoadCurrent;
}

void AN_doAccumulation(void)
{
	static int32_t lastCurrent, t1;
	static uint32_t t2;
	
	t1 = AN_getBattCurrent();
	t2 = HAL_GetTick();
	battAcumCharge_mC += ((lastCurrent+t1) * (t2 - lastAcumTick))/2000;
	lastAcumTick = t2;
	lastCurrent = t1;
}

int32_t AN_getBattCharge(int8_t doClear)
{
	static int32_t t1;
	
	t1 = battAcumCharge_mC;
	
	if (doClear) battAcumCharge_mC = 0;
	
	return t1;
}








