#ifndef analog_h
#define analog_h

#define AN_num_Vbatt					2
#define AN_num_Vsolar					4
#define AN_num_Vload					3
#define AN_num_Isolar					1
#define AN_num_Iload					0
#define AN_num_Temp						6
#define AN_num_Intref					5

#define VREFINT_CAL_ADDR    	0x1FF80078
#define FULL_SCALE						0xFFF

#define RSENSE_MOHM_L						3
#define RSENSE_MOHM_S						10
#define ISENSE_GAIN							100
#define VOLTTOAMP_MOHM_L				(RSENSE_MOHM_L * ISENSE_GAIN)
#define VOLTTOAMP_MOHM_S				(RSENSE_MOHM_S * ISENSE_GAIN)

#define AN_doAccumulation_T		10


/**
	* Public function definitions
	*/
void AN_Init(void);
int32_t AN_getBattVoltage(void);
int32_t AN_getLoadVoltage(void);
int32_t AN_getBattCurrent(void);
int32_t AN_getSolarPower(void);
int32_t AN_getLoadPower(void);
int32_t AN_getLoadCurrent(void);
void AN_doAccumulation(void);
int32_t AN_getBattCharge(int8_t);

//int32_t AN_getSolarVoltage(void);
//int32_t AN_get


#endif