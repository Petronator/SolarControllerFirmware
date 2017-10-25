#ifndef led_h
#define led_h

#define LED_stepDisplay_T				250

void LED_startDisplay(void);
void LED_setState(uint8_t, uint8_t);		//green, red
void LED_stepDisplay(void);

#endif