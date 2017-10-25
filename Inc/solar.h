#ifndef solar_h
#define solar_h

void Solar_SetToStandby(void);
void Solar_StepDownPWM(void);
void Solar_StepUpPWM(void);
void Solar_EnablePWM(void);
void Solar_AdjPWM(void);

#define Solar_DoAdj_T									5

#endif