#include "grbl.h"

// throttle defines
#define DTP_MIN 768 /* 0% */
#define DTP_LSPLIT 682 /* 25% */
#define DTP_USPLIT 352 /* 75% */
#define DTP_MAX 304 /* 100% */
#define THROTTLE_DEBOUNCE_COUNTER 250

// handy macros
#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))
#define map(x,in_min,in_max,out_min,out_max) (x-in_min)*(out_max-out_min)/(in_max-in_min)+out_min

volatile uint8_t previousState = 0;
volatile uint8_t currentState = 0;

uint8_t currentThrottlePosition = 0;
uint8_t lastThrottlePosition = 0;
uint8_t throttleDebounceCounter = 0;

// over sample the throttle to smooth it out
#define THROTTLE_SAMPLING 4
uint16_t throttleSamples[THROTTLE_SAMPLING];

void input_init() {
  // set up A/D converter for one input
  bit_true(ADCSRA, bit(ADEN)); // Enable the ADC.
  // faster analog read
  // http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1208715493/15
  // set prescale to 16 (fast analog read)
  //bit_true(ADCSRA, bit(ADPS2));
  //bit_false(ADCSRA, bit(ADPS1));
  //bit_false(ADCSRA, bit(ADPS0));
  // select ADC7 (A7)
  bit_true(ADMUX, bit(MUX0));
  bit_true(ADMUX, bit(MUX1));
  bit_true(ADMUX, bit(MUX2));
  bit_false(ADMUX, bit(ADLAR)); // Make sure the ADC value is right-justified.
  bit_true(ADMUX, bit(6)); // analog reference to default

  // configure inputs, set bits to 0
  bit_false(DDRD, bit(PD7)); // UP
  bit_false(DDRC, bit(PC0)); // DN
  bit_false(DDRC, bit(PC1)); // LEFT
  bit_false(DDRC, bit(PC2)); // RIGHT
  bit_false(DDRC, bit(PC3)); // IN
  bit_false(DDRC, bit(PC4)); // OUT
  bit_false(DDRC, bit(PC5)); // THROTTLE BUTTON
  bit_false(DDRA, bit(PA7)); // POT

  // enable interrupt on pins
  bit_true(PCMSK2, bit(PC0) | bit(PC1) | bit(PC2) | bit(PC3) | bit(PC4) | bit(PC5));
  bit_true(PCMSK3, bit(PD7));

  // enable interrupt on ports
  // PCIE2 also enabled via limits
  // PCIE0 also enabled by system
  bit_true(PCICR, bit(PCIE2) | bit(PCIE3));

  // clear throttle buffer
  for (uint8_t i = 0; i < THROTTLE_SAMPLING; i++) {
    throttleSamples[i] = 768; // lowest position;
  }

}

int input_high(uint8_t dinIndex) {
  return currentState & bit(dinIndex);
}

int input_edgeHigh(uint8_t dinIndex) {
  return (bit_istrue(currentState, bit(dinIndex))
      && bit_isfalse(previousState, bit(dinIndex)));
}

int input_edgeLow(uint8_t dinIndex) {
  return (bit_isfalse(currentState, bit(dinIndex))
      && bit_istrue(previousState, bit(dinIndex)));
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

void input_readDigital() {
  currentState = 0;
  // read all pin states in
  uint8_t pinValuesA = PINA;
  uint8_t pinValuesC = PINC;
  uint8_t pinValuesD = PIND;

  // copy pin states to one byte
  if (pinValuesD & bit(PD7)) bit_true(currentState, bit(BTN_IDX_UP));
  if (pinValuesC & bit(PC0)) bit_true(currentState, bit(BTN_IDX_DN));
  if (pinValuesC & bit(PC1)) bit_true(currentState, bit(BTN_IDX_LF));
  if (pinValuesC & bit(PC2)) bit_true(currentState, bit(BTN_IDX_RT));
  if (pinValuesC & bit(PC3)) bit_true(currentState, bit(BTN_IDX_IN));
  if (pinValuesC & bit(PC4)) bit_true(currentState, bit(BTN_IDX_OT));
  if (pinValuesC & bit(PC5)) bit_true(currentState, bit(BTN_IDX_TH));
  if (pinValuesA & bit(PA0)) bit_true(currentState, bit(BTN_IDX_SP));

}

void input_clear() {
  previousState = currentState;
}

void input_readAnalog() {
  // read throttle
  if (throttleDebounceCounter > 0) {
    throttleDebounceCounter--;
  } else {
    // read the last analog value converted, add to buffer
    int low = ADCL;
    int high = ADCH;
    int pot = (high << 8) | low;

    // shift sampling buffer left
    for (uint8_t i = 1; i < THROTTLE_SAMPLING; i++) {
      throttleSamples[i - 1] = throttleSamples[i];
    }
    // add new value
    throttleSamples[THROTTLE_SAMPLING - 1] = pot;

    // take average
    uint16_t total = 0;
    for (uint8_t i = 0; i < THROTTLE_SAMPLING; i++) {
      total += throttleSamples[i];
    }
    pot = total / THROTTLE_SAMPLING;

    // flatten the curve produced by throttle pot
    if (pot > DTP_LSPLIT) pot = map(pot, DTP_MIN, DTP_LSPLIT, 0, 30);
    else if (pot > DTP_USPLIT) pot = map(pot, DTP_LSPLIT, DTP_USPLIT, 30, 90);
    else pot = map(pot, DTP_USPLIT, DTP_MAX, 90, 120);

    // divide into sixths
    uint8_t newThrottlePosition = constrain(pot / 20, 0, 5);
    lastThrottlePosition = currentThrottlePosition;
    currentThrottlePosition = newThrottlePosition;
    throttleDebounceCounter = THROTTLE_DEBOUNCE_COUNTER;
  }

  bit_true(ADCSRA, bit(ADSC)); // queue the next analog read
}
