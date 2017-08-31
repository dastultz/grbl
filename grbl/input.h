#ifndef input_h
#define input_h

// initialize
void input_init();

// service the inputs
// read all digital and analog inputs
void input_service();

// todo: is there a signed 8-bit type that will work for these boolean functions?
// see plan_check_full_buffer

// is the digital pin on
int input_high(uint8_t dinIndex);

// did the pin just go high
int input_edgeHigh(uint8_t dinIndex);

// did the pin just go low
int input_edgeLow(uint8_t dinIndex);

// did the throttle position change
int input_throttlePositionChanged();

/// position of throttle 0 to 5
uint8_t input_throttlePosition();

// first pressed button, if any
uint8_t input_firstPressedAxisButton();

int input_debounceInterval();

#endif
