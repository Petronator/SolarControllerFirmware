/**
	* This file contains code that controls the onboard indicator LED. The LED performs
	* flash patterns to indicate current power and battery SOC. 
	*
	* Flash pattern consists of "short" pulses (counting for one unit) and "long" pulses
	* (counting for 5 units), separated by an off period.
	* Pattern displays green for solar power, yellow for SOC, and red for load power, in
	* that order. Each measurement is separated by a longer off period.
	*/
	
#include "main.h"
#include "stm32l0xx_hal.h"
#include "analog.h"
#include "led.h"
#include "battery.h"


#define offticks		250
#define transticks	750
#define onticks			250
#define longticks		500



//uint32_t starttick;
uint32_t nextstep;							//tick at which the next transition will occur
uint16_t solarpulses;						//pulses to be displayed for solar
uint16_t batterypulses;					//...for battery
uint16_t loadpulses;						//...for load
uint16_t pulsesElapsed;					//Number of pulses displayed since starting the sequence
uint8_t ledIsOn;								//True if last action was enabling LED

void LED_startDisplay(void)
{
	solarpulses = (AN_getSolarPower()+500)/1000;		//1 pulse = 1 Watt
	//batterypulses = (Battery_getSOC()+5)/10;			//1 pulse = 10% SOC
	batterypulses = (AN_getBattVoltage()+100) / 200; //1 pulse = 200 mV
	loadpulses = (AN_getLoadPower()+500)/1000;			//1 pulse = 1 Watt
	
	//solarpulses = 6;
	//batterypulses = 6;
	//loadpulses = 2;
	
	pulsesElapsed = 0;
	ledIsOn = 0;
	
	nextstep = HAL_GetTick()-1;
	LED_stepDisplay();
	
}

void LED_stepDisplay(void)
{	
	//check to see if we're done
	if (ledIsOn == 0 && pulsesElapsed >= solarpulses + batterypulses + loadpulses)
		return;
	
	//if it's not time to transition yet, return immediately
	if (HAL_GetTick() < nextstep) return;
	
	//Otherwise, determine the next state
	if (ledIsOn)
	{
		//turn off the LED
		LED_setState(0, 0);
		ledIsOn = 0;
				
		//check if we're waiting for more pulses or the next measurement
		if (pulsesElapsed == solarpulses || pulsesElapsed == solarpulses+batterypulses)
		{
			nextstep = nextstep + transticks;
		}
		else
		{
			nextstep = nextstep + offticks;
		}
	}
	else
	{
		if (pulsesElapsed < solarpulses)
		{
			//Light Green for solar pulses
			LED_setState(1, 0);
			ledIsOn = 1;
			//check if we should do a long pulse or short pulse
			if (pulsesElapsed + 5 <= solarpulses)
			{
				nextstep = nextstep + longticks;
				pulsesElapsed += 5;
			}
			else
			{
				nextstep = nextstep + onticks;
				pulsesElapsed += 1;
			}
		}
		else if (pulsesElapsed - solarpulses < batterypulses)
		{
			//Light yellow for battery pulses
			LED_setState(1, 1);
			ledIsOn = 1;
			if (pulsesElapsed - solarpulses + 5 <= batterypulses)
			{
				nextstep = nextstep + longticks;
				pulsesElapsed += 5;
			}
			else
			{
				nextstep = nextstep + onticks;
				pulsesElapsed += 1;
			}
		}
		else
		{
			//Light Red for load pulses
			LED_setState(0, 1);
			ledIsOn = 1;
			if (pulsesElapsed - solarpulses - batterypulses + 5 <= loadpulses)
			{
				nextstep = nextstep + longticks;
				pulsesElapsed += 5;
			}
			else
			{
				nextstep = nextstep + onticks;
				pulsesElapsed += 1;
			}
		}
	}
	
}

/** Set the Red/Green LED to the indicated state (1 is on, 0 is off)
	*/

void LED_setState(uint8_t green, uint8_t red)
{
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, 1-red);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, 1-green);
}


