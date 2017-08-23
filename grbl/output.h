#ifndef output_h
#define output_h

// initialize
void output_init();

// service the outputs
// changes are queued and actually written here
void output_service();

// turn pin on
void output_setHigh(uint8_t doutIndex);

// turn pin off
void output_setLow(uint8_t doutIndex);

// turn on one light, all others out
void output_exclusiveLightOn(uint8_t light);

// turn all lights off
void output_clearAllLights();

#endif
