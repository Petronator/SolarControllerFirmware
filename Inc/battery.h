/**
	* Code to manage the charger's internal Battery
	*/
	
#ifndef __BATTERY_H
#define __BATTERY_H

#define CAPACITY_MAH								2000
#define BATTERY_MAX_CURRENT_MA			2000

#define BATTERY_EOC_VOLTAGE_MV			4100
#define BATTERY_EOD_VOLTAGE_MV			3200

#define BATTERY_FALLOFF_THRESHOLD		100

#define Battery_estimateSOC_T				1000


//Function Prototypes

void Battery_estimateSOC(void);

int16_t Battery_getSOC(void);

int32_t Battery_getChargeReq(void);
int32_t Battery_getMaxCharge(void);
int32_t Battery_getMaxDischarge(void);

#endif