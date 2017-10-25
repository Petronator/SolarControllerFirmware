#ifndef load_h
#define load_h

//Instant current at which the load will immediately turn off. Set low for testing.
#define LOAD_CURRENT_HARD_LIMIT_mA		000

#define USB_NOLOAD_TARGETVOLT_MV			5100

#define Load_DoAdj_T									1

#define MAX_DUTY_CYCLE								60

void Load_AdjPWM(void);
void Load_Enable(void);
void Load_Disable(void);
uint16_t Load_IsEnabled(void);

#endif