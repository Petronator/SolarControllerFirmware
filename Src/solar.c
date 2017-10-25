/**
	* solar.c
	* Control of Solar buck converter PWM
	*/
	
#include "main.h"
#include "stm32l0xx_hal.h"
#include "analog.h"
#include "battery.h"
#include "solar.h"
#include "led.h"
	

uint8_t Solar_laststepwasup = 0;
uint8_t Solar_PWMEnabled = 0;
int32_t Solar_PWM;


void Solar_EnablePWM(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;
	
	/**
		* SOLAR BUCK CONVERTER PWM OPERATION
		* Period is 20 us, divided into 160 ticks at 1/8 us each.
		* High side FET operates for first part of cycle.
		* To guarantee Q1>Q2 dead time, 1 tick is spent with both off (diode conducts)
		* Remainder of cycle is spent with Q2 on.
		* Q2>Q1 dead time guaranteed by circuit characteristics. No SW dead time needed.
		*/
	
	//Enable PWM pins
	//Q1 Init
	GPIO_InitStruct.Pin = GPIO_PIN_3;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF2_TIM2;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	//Q2 Init
	GPIO_InitStruct.Pin = GPIO_PIN_2;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF2_TIM2;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	
	//Set up PWM for a constant frequency of 50 kHz
	//(Might have already been done by the load controller.)
	//(both controllers run on the same freq. so use the same timer)
	//50 kHz -> 20 us
	
	TIM2->PSC = 0;				//Prescaler: ticks every 1/8 us
	TIM2->ARR = 159	;			//Ticks per period, 1/8 us * 160 = 20 us
	
	//Initialize gain 
	Solar_PWM = 159;
	TIM2->CCR4 = Solar_PWM;
	TIM2->CCR3 = Solar_PWM+1;
	
	//Q1 Init
	TIM2->CCMR2 |= TIM_CCMR2_OC4M_2 | TIM_CCMR2_OC4M_1
						| TIM_CCMR2_OC4PE | TIM_CCMR2_OC4FE; /* (4) */
	TIM2->CCER |= TIM_CCER_CC4P;	//negative polarity (low Q6 enables Q1)
	//Q2 Init
	TIM2->CCMR2 |= TIM_CCMR2_OC3M_2 | TIM_CCMR2_OC3M_1
						| TIM_CCMR2_OC3PE | TIM_CCMR2_OC3FE; /* (4) */
	TIM2->CCER |= TIM_CCER_CC3P;	//negative polarity (enabled on second half)

	TIM2->EGR |= TIM_EGR_UG; /* (7) */
	//Q1 and Q2 Start
	TIM2->CR1 |= TIM_CR1_CEN; /* (6) */
	TIM2->CCER |= TIM_CCER_CC4E | TIM_CCER_CC3E; /* (5) */
	
	Solar_PWMEnabled = 1;
	
	//Debugging: Light LED
	LED_setState(1,0);
}
void Solar_SetToStandby(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;
	
	//Disable PWM, turn on the high gate
	GPIO_InitStruct.Pin = GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET);
	
	GPIO_InitStruct.Pin = GPIO_PIN_2;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_RESET);
	
	
	TIM2->CCER &= ~(TIM_CCER_CC4E | TIM_CCER_CC3E);
	
	
	Solar_PWMEnabled = 0;
}
void Solar_StepDownPWM(void)
{
	Solar_PWM--;
	if (Solar_PWM < 0) Solar_PWM = 0;
	
	TIM2->CCR4 = Solar_PWM;
	TIM2->CCR3 = Solar_PWM+1;
	Solar_laststepwasup = 0;
}
void Solar_StepUpPWM(void)
{
	Solar_PWM++;
	if (Solar_PWM > 160) Solar_PWM = 160;
	
	TIM2->CCR3 = Solar_PWM+1;
	TIM2->CCR4 = Solar_PWM;
	Solar_laststepwasup = 1;
}



/** Adjust the PWM of the solar converter
	*
	* This function implements perturb-and-observe while respecting the battery's
	* current limit. 
	*/
void Solar_AdjPWM(void)
{
	static uint32_t spower, Solar_lastPower;
	int32_t t1, t2;
	
	spower = AN_getSolarPower();
	
	//Check if we're operating but not pushing power
	if (Solar_PWMEnabled && spower < 100 && Battery_getMaxCharge() > 100)
	{
		//In this case, we disable the PWM
		Solar_SetToStandby();
	}
	//Check if we're not operating but starting conducting current
	else if (!Solar_PWMEnabled && spower > 500)
	{
		Solar_EnablePWM();
	}
	else if (!Solar_PWMEnabled)
	{
		//This might not be needed. Consider reimplementing if standby mode doesn't hold Q1 on.
		
		//In order to keep Q1 open, we need to pulse the PWM
		//This maintains the bootstrap voltage
		//Turn off Q1, turn on Q2, turn off Q2, turn on Q1
		//HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_SET);
		//HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_SET);
		//for(t1 = 0; t1 < 0; t1++);
		//HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_RESET);
		//HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET);
		
		return;
	}
	//If the battery current is too high, we need to step down
	//t1 = AN_getBattCurrent();
	//t2 = Battery_getMaxCharge();
	if (AN_getBattCurrent() >= Battery_getMaxCharge())
	{
		Solar_StepDownPWM();
	}
	//Otherwise, do perturb-and-observe
	else if (spower > Solar_lastPower)
	{
		//power increasing; continue in same direction
		if (Solar_laststepwasup)
		{
			Solar_StepUpPWM();
		}
		else
		{
			Solar_StepDownPWM();
		}
	}
	else
	{
		//power decreasing; reverse direction
		if (Solar_laststepwasup)
		{
			Solar_StepDownPWM();
		}
		else
		{
			Solar_StepUpPWM();
		}
	}
	Solar_lastPower = spower;
	
}



