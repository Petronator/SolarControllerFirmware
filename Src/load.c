/**
	* load.c
	* Control of load boost converter PWM
	*
	* This control scheme is a two-level proportional/integral control.
	* A power target is set based on the error in voltage,
	* then the PWM is adjusted based on the error in power.
	*
	* 
	*
	* 300 mV error -> 10 W
	* 3 mV error -> 100 mW
	*/
	
#include "main.h"
#include "stm32l0xx_hal.h"
#include "analog.h"
#include "battery.h"
#include "load.h"
#include "led.h"



uint16_t loadEnabled = 0;
uint32_t dutyCycle_M = 0;
int32_t Load_Target_mV = USB_NOLOAD_TARGETVOLT_MV;

	
void Load_AdjPWM(void)
{
	static int32_t load_power_target, load_pwm_comp, t1, t2, t3;
	
	if (!loadEnabled) return;
	
	t1 = AN_getLoadCurrent();
	t2 = AN_getLoadVoltage();
	t3 = AN_getLoadPower();
	
	if (t1 > LOAD_CURRENT_HARD_LIMIT_mA)
	{
		//Hard Stop, consider adding some error code
		Load_Disable();
		return;
	}
	if ((-1 * AN_getBattCurrent()) >= Battery_getMaxDischarge())
	{
		//This will cause a "soft stop" by reducing the target load voltage
		//Hopefully this leads to the USB device reducing power consumption
		Load_Target_mV -= 20;
		if (Load_Target_mV < 0) 
			Load_Target_mV = 0;
	}
	else if (Load_Target_mV < USB_NOLOAD_TARGETVOLT_MV)
	{
		Load_Target_mV += 1;
	}
	
	
	t1 = AN_getLoadVoltage();
	t2 = AN_getLoadPower();
	
	//Outer loop: determine target load power
	load_power_target = ((Load_Target_mV - AN_getLoadVoltage()) * 30);
	
	//Inner loop: correct PWM to compensate
	load_pwm_comp = (load_power_target - AN_getLoadPower());
	
	//Apply correction
	if ((int32_t)dutyCycle_M < (-1 * load_pwm_comp))
		dutyCycle_M = 0;
	else
		dutyCycle_M += (load_pwm_comp)/50;
	
	if (dutyCycle_M > (MAX_DUTY_CYCLE*10000))
		dutyCycle_M = (MAX_DUTY_CYCLE*10000);
	
	TIM2->CCR1 = 0;
	TIM2->CCR2 = 160-((dutyCycle_M) / 6250)+1;
	TIM2->CCR1 = 160-((dutyCycle_M) / 6250);
	
}

void Load_Enable(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;
	
	//Enable PWM pins
	GPIO_InitStruct.Pin = GPIO_PIN_3;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF2_TIM2;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
	
	GPIO_InitStruct.Pin = GPIO_PIN_15;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF2_TIM2;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	
	//Set up PWM for a constant frequency of 50 kHz
	//50 kHz -> 20 us
	
	TIM2->PSC = 0;				//Prescaler: ticks every 1/8 us
	TIM2->ARR = 159	;			//Ticks per period, 1/8 us * 160 = 20 us
	
	//Initialize gain based on ideal boost converter formula
	//dutyCycle_M = (1000-(1000 * AN_getBattVoltage()) / USB_NOLOAD_TARGETVOLT_MV)*1000;
	//TIM2->CCR2 = (dutyCycle_M) / 6250;
	dutyCycle_M = 0;
	TIM2->CCR2 = 160;
	TIM2->CCR1 = 160;
	
	TIM2->CCMR1 |= TIM_CCMR1_OC2M_2 | TIM_CCMR1_OC2M_1
						| TIM_CCMR1_OC2PE | TIM_CCMR1_OC2FE; /* (4) */
	TIM2->CCMR1 |= TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1
						| TIM_CCMR1_OC1PE | TIM_CCMR1_OC1FE; /* (4) */
						
	TIM2->CCER |= TIM_CCER_CC2P | TIM_CCER_CC1P;
	
	TIM2->EGR |= TIM_EGR_UG; /* (7) */
	
	TIM2->CCER |= TIM_CCER_CC2E | TIM_CCER_CC1E; /* (5) */
	TIM2->CR1 |= TIM_CR1_CEN; /* (6) */
	
	LED_setState(0,0);
	
	loadEnabled = 1;
}
void Load_Disable(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;
	int32_t t1,t2,t3,t4;
	
	//initialize "ground" pin (part of rework)
	GPIO_InitStruct.Pin = GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	//HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	//HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_RESET);
	
	
	//Shove the GPIO bit to manual output mode, clear it
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_SET);
	
	GPIO_InitStruct.Pin = GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);
	
	
	
	GPIO_InitStruct.Pin = GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_SET);
	
	//Then turn off the PWM
	TIM2->CCER &= ~(TIM_CCER_CC2E | TIM_CCER_CC1E);
	
	//debugging: turn on the LED
	LED_setState(0,1);
	
	//Turn Q3 On to prevent idle power drain
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_SET);
	
	loadEnabled = 0;
	
	//Debug information
	t1 = AN_getBattVoltage();
	t2 = AN_getLoadVoltage();
	t3 = AN_getLoadCurrent();
	t4 = 1;
}

uint16_t Load_IsEnabled(void)
{
	return loadEnabled;
}