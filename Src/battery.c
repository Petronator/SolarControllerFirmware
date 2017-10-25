/**
	* Code to manage the charger's internal Battery
	*/
	
#include "main.h"
#include "stm32l0xx_hal.h"
#include "battery.h"
#include "analog.h"

// Global Variable Definitions

int32_t Battery_Charge_mC = CAPACITY_MAH * 3600 / 2;
int16_t Battery_SOC_pct = 50;

int Battery_EOD_Flag = 0;
int Battery_EOC_Flag = 0;

	
	/** Estimate the remaining state of charge of the battery
*/
	void Battery_estimateSOC(void)
	{
		//For now, just use Ah count
		Battery_Charge_mC += AN_getBattCharge(1);
		
		if (AN_getBattVoltage() <= BATTERY_EOD_VOLTAGE_MV)
		{
			//Reset to 0
			Battery_Charge_mC = 0;
			Battery_EOD_Flag = 1;
		}
		else if (AN_getBattVoltage() >= BATTERY_EOC_VOLTAGE_MV)
		{
			//Reset to 100%
			Battery_Charge_mC = CAPACITY_MAH * 3600;
			Battery_EOC_Flag = 1;
		}
		else
		{
			Battery_EOD_Flag = 0;
			Battery_EOC_Flag = 0;
		}
		
		Battery_SOC_pct = (Battery_Charge_mC/36)/CAPACITY_MAH;
		
	}
	
int16_t Battery_getSOC(void)
{
	return Battery_SOC_pct;
}

int32_t Battery_getChargeReq(void)
{
	//This value indicates the minimum charge power we should have available
	//before allowing the load to be enabled
	
	//Set to max charge current at 0%, then slope down to 0 at 20%
	int32_t t1 = BATTERY_MAX_CURRENT_MA - (Battery_SOC_pct * BATTERY_MAX_CURRENT_MA)/20;
	
	return t1>0? t1 : 0;
}

int32_t Battery_getMaxCharge(void)
{
	int32_t battVolt;
	
	battVolt = AN_getBattVoltage();
	
	if (battVolt >= BATTERY_EOC_VOLTAGE_MV || Battery_EOC_Flag)
	{
		return 0;
	}
	else if (battVolt <= BATTERY_EOC_VOLTAGE_MV - BATTERY_FALLOFF_THRESHOLD)
	{
		return BATTERY_MAX_CURRENT_MA;
	}
	else
	{
		return (BATTERY_MAX_CURRENT_MA *(BATTERY_EOC_VOLTAGE_MV-battVolt))
			/ BATTERY_FALLOFF_THRESHOLD;
	}
}	

/** Battery_getMaxDischarge
  * 
	* Returns that maximum allowed discharge current.
	* This drops off when the battery is at a low voltage. Returns 0 at EOD.
	*/

int32_t Battery_getMaxDischarge(void)
{
	int32_t battVolt;
	
	battVolt = AN_getBattVoltage();
	
	if (battVolt <= BATTERY_EOD_VOLTAGE_MV || Battery_EOD_Flag)
	{
		return 0;
	}
	else if (battVolt >= BATTERY_EOD_VOLTAGE_MV + BATTERY_FALLOFF_THRESHOLD)
	{
		return BATTERY_MAX_CURRENT_MA;
	}
	else
	{
		return (BATTERY_MAX_CURRENT_MA *(battVolt-BATTERY_EOD_VOLTAGE_MV))
			/ BATTERY_FALLOFF_THRESHOLD;
	}
}




