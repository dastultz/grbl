#include "grbl.h"

#define DOUT_STATE 0b001
#define DOUT_CHANGE 0b010

// index into array of digital outputs
#define OUT_IDX_UP 0
#define OUT_IDX_DN 1
#define OUT_IDX_LF 2
#define OUT_IDX_RT 3
#define OUT_IDX_IN 4
#define OUT_IDX_OT 5
#define OUT_IDX_BZ 6

#define PORT_NUM_A 0
#define PORT_NUM_B 1
#define PORT_NUM_C 2
#define PORT_NUM_D 3

uint8_t outputStates[] = { 0, 0, 0, 0, 0, 0, 0 };

void output_init() {

  // configure outputs, set bits to 1
  DDRA |= 1 << PA1; // BUZZER
  DDRB |= 1 << PB7; // IN
  DDRD |= 1 << PD6; // RIGHT
  DDRD |= 1 << PD5; // OUT
  DDRD |= 1 << PD4; // LEFT
  DDRD |= 1 << PD3; // DN
  DDRD |= 1 << PD2; // UP

  // set up ports
  outputStates[OUT_IDX_UP] |= PORT_NUM_D << 2;
  outputStates[OUT_IDX_DN] |= PORT_NUM_D << 2;
  outputStates[OUT_IDX_LF] |= PORT_NUM_D << 2;
  outputStates[OUT_IDX_RT] |= PORT_NUM_D << 2;
  outputStates[OUT_IDX_IN] |= PORT_NUM_B << 2;
  outputStates[OUT_IDX_OT] |= PORT_NUM_D << 2;
  outputStates[OUT_IDX_BZ] |= PORT_NUM_A << 2;

  // set up bits
  outputStates[OUT_IDX_UP] |= PD2 << 4;
  outputStates[OUT_IDX_DN] |= PD3 << 4;
  outputStates[OUT_IDX_LF] |= PD4 << 4;
  outputStates[OUT_IDX_RT] |= PD6 << 4;
  outputStates[OUT_IDX_IN] |= PB7 << 4;
  outputStates[OUT_IDX_OT] |= PD5 << 4;
  outputStates[OUT_IDX_BZ] |= PA1 << 4;

}

void output_setHigh(uint8_t dinIndex) {
  outputStates[dinIndex] |= DOUT_STATE; // set state bit high
  outputStates[dinIndex] |= DOUT_CHANGE; // raise pending change bit
}

void output_setLow(uint8_t dinIndex) {
  outputStates[dinIndex] &= ~DOUT_STATE; // set state bit low
  outputStates[dinIndex] |= DOUT_CHANGE; // raise pending change bit
}

void output_service() {
  for (uint8_t i = 0; i < 7; i++) {
    // if no change pending, move on to next
    if (! (outputStates[i] & DOUT_CHANGE)) {
      continue;
    }
    outputStates[i] &= ~DOUT_CHANGE; // clear pending change bit
    uint8_t bit = 1 << ((outputStates[i] & 0b1110000) >> 4);
    uint8_t port = (outputStates[i] & 0b1100) >> 2;
    switch (port) {
      case PORT_NUM_A:
        if (outputStates[i] & DOUT_STATE) PORTA |= bit;
        else PORTA &= ~bit;
        break;
      case PORT_NUM_B:
        if (outputStates[i] & DOUT_STATE) PORTB |= bit;
        else  PORTB &= ~bit;
        break;
      case PORT_NUM_C:
        if (outputStates[i] & DOUT_STATE) PORTC |= bit;
        else PORTC &= ~bit;
        break;
      case PORT_NUM_D:
        if (outputStates[i] & DOUT_STATE) PORTD |= bit;
        else PORTD &= ~bit;
        break;
    }
  }
}

void output_exclusiveLightOn(uint8_t light) {
  output_clearAllLights();
  output_setHigh(light);
}

void output_clearAllLights() {
  for (uint8_t i = 0; i < 6; i++) {
    output_setLow(i);
  }
}
