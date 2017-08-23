#include "grbl.h"

#define DEBOUNCE_INTERVAL 250

#define DIN_STATE_CURR 0b001
#define DIN_STATE_PREV 0b010

#define PORT_NUM_A 0
#define PORT_NUM_B 1
#define PORT_NUM_C 2
#define PORT_NUM_D 3

// throttle defines
#define ANALOG_THROTTLE 7
#define DTP_MIN 768 /* 0% */
#define DTP_LSPLIT 682 /* 25% */
#define DTP_USPLIT 352 /* 75% */
#define DTP_MAX 304 /* 100% */

// handy macros
#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))
#define map(x,in_min,in_max,out_min,out_max) (x-in_min)*(out_max-out_min)/(in_max-in_min)+out_min


// defines for setting and clearing register bits
// todo: see nuts_bolts.h for bit macros
#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

uint8_t buttonStates[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
uint8_t debounceCounters[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
uint8_t currentThrottlePosition = 0;
uint8_t lastThrottlePosition = 0;
uint8_t throttleDebounceCounter = 0;

void input_init() {
  // set up A/D converter for one input
	sbi(ADCSRA, ADEN); // Enable the ADC.
  // faster analog read
  // http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1208715493/15
  // set prescale to 16 (fast analog read)
	//sbi(ADCSRA, ADPS2);
	//cbi(ADCSRA, ADPS1);
	//cbi(ADCSRA, ADPS0);
	// select ADC7 (A7)
	sbi(ADMUX, MUX0);
	sbi(ADMUX, MUX1);
	sbi(ADMUX, MUX2);
	cbi(ADMUX, ADLAR); // Make sure the ADC value is right-justified.
	sbi(ADMUX, 6); // analog reference to default

  // todo: use sbi and cbi?
  // configure inputs, set bits to 0
  DDRA &= ~(1 << PA0); // THERMAL (SPINDLE)
  DDRC &= ~(1 << PC7); // LIMIT
  DDRC &= ~(1 << PC6); // E-STOP
  DDRC &= ~(1 << PC5); // THROTTLE BUTTON
  DDRC &= ~(1 << PC4); // OUT
  DDRC &= ~(1 << PC3); // IN
  DDRC &= ~(1 << PC2); // RIGHT
  DDRC &= ~(1 << PC1); // LEFT
  DDRC &= ~(1 << PC0); // DN
  DDRD &= ~(1 << PD7); // UP
  DDRA &= ~(1 << PA7); // POT

  // set up ports
  buttonStates[BTN_IDX_UP] |= PORT_NUM_D << 2;
  buttonStates[BTN_IDX_DN] |= PORT_NUM_C << 2;
  buttonStates[BTN_IDX_LF] |= PORT_NUM_C << 2;
  buttonStates[BTN_IDX_RT] |= PORT_NUM_C << 2;
  buttonStates[BTN_IDX_IN] |= PORT_NUM_C << 2;
  buttonStates[BTN_IDX_OT] |= PORT_NUM_C << 2;
  buttonStates[BTN_IDX_TH] |= PORT_NUM_C << 2;
  buttonStates[BTN_IDX_ES] |= PORT_NUM_C << 2;
  buttonStates[BTN_IDX_LM] |= PORT_NUM_C << 2;
  buttonStates[BTN_IDX_SP] |= PORT_NUM_A << 2;

  // set up bits
  buttonStates[BTN_IDX_UP] |= PD7 << 4;
  buttonStates[BTN_IDX_DN] |= PC0 << 4;
  buttonStates[BTN_IDX_LF] |= PC1 << 4;
  buttonStates[BTN_IDX_RT] |= PC2 << 4;
  buttonStates[BTN_IDX_IN] |= PC3 << 4;
  buttonStates[BTN_IDX_OT] |= PC4 << 4;
  buttonStates[BTN_IDX_TH] |= PC5 << 4;
  buttonStates[BTN_IDX_ES] |= PC6 << 4;
  buttonStates[BTN_IDX_LM] |= PC7 << 4;
  buttonStates[BTN_IDX_SP] |= PA0 << 4;

}

int input_high(uint8_t dinIndex) {
  return buttonStates[dinIndex] & DIN_STATE_CURR;
}

int input_edgeHigh(uint8_t dinIndex) {
  if (bit_istrue(buttonStates[dinIndex], DIN_STATE_CURR)
      && bit_isfalse(buttonStates[dinIndex], DIN_STATE_PREV)) {
    return 1;
  } else {
    return 0;
  }
}

int input_edgeLow(uint8_t dinIndex) {
  if (bit_isfalse(buttonStates[dinIndex], DIN_STATE_CURR)
      && bit_istrue(buttonStates[dinIndex], DIN_STATE_PREV)) {
    return 1;
  } else {
    return 0;
  }
}

int input_throttlePositionChanged() {
  if (throttleDebounceCounter > 0) return 0;
  return currentThrottlePosition != lastThrottlePosition;
}

uint8_t input_throttlePosition() {
  return currentThrottlePosition;
}

uint8_t input_firstEdgeHighAxisButton() {
  for (uint8_t i = 0; i < 6; i++) {
    if (input_edgeHigh(i)) return i;
  }
  return 6;
}

void input_service() {
  // read digital inputs
  for (uint8_t i = 0; i < 10; i++) {
    // copy current state to previous state
    buttonStates[i] &= 0b1111101;
    uint8_t oldState = buttonStates[i] & DIN_STATE_CURR;
    buttonStates[i] |= oldState << 1;

    // if this input is currently debouncing, decrement and go to next
    if (debounceCounters[i] > 0) {
      debounceCounters[i] -= 1;
      continue;
    }

    // read in new state
    buttonStates[i] &= 0b1111110; // clear current state
    uint8_t bitMask = 1 << ((buttonStates[i] & 0b1110000) >> 4);
    uint8_t port = (buttonStates[i] & 0b1100) >> 2;
    uint8_t state = 0;
    switch (port) {
      case PORT_NUM_A:
        state = PINA & bitMask;
        break;
      case PORT_NUM_B:
        state = PINB & bitMask;
        break;
      case PORT_NUM_C:
        state = PINC & bitMask;
        break;
      case PORT_NUM_D:
        state = PIND & bitMask;
        break;
    }
    if (state) buttonStates[i] |= 1;

    // start debounce counter for this input
    if (input_edgeHigh(i) || input_edgeLow(i)) {
      debounceCounters[i] = DEBOUNCE_INTERVAL;
    }

  }

  if (throttleDebounceCounter > 0) {
    throttleDebounceCounter--;
  } else {
    // read the last analog value
    int low = ADCL;
    int high = ADCH;
    int pot = (high << 8) | low;

    // flatten the curve produced by throttle pot
    if (pot > DTP_LSPLIT) pot = map(pot, DTP_MIN, DTP_LSPLIT, 0, 30);
    else if (pot > DTP_USPLIT) pot = map(pot, DTP_LSPLIT, DTP_USPLIT, 30, 90);
    else pot = map(pot, DTP_USPLIT, DTP_MAX, 90, 120);

    // divide into sixths
    uint8_t newThrottlePosition = constrain(pot / 20, 0, 5);
    lastThrottlePosition = currentThrottlePosition;
    currentThrottlePosition = newThrottlePosition;
    throttleDebounceCounter = DEBOUNCE_INTERVAL;
  }

  sbi(ADCSRA, ADSC); // queue the next analog read
}
