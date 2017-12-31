#include "grbl.h"

#define DOUT_STATE 0b001
#define DOUT_CHANGE 0b010

#define PORT_NUM_A 0
#define PORT_NUM_B 1
#define PORT_NUM_C 2
#define PORT_NUM_D 3

uint8_t outputStates[] = { 0, 0, 0, 0, 0, 0, 0 };

void output_init() {

  // configure outputs, set bits to 1
  bit_true(DDRD, bit(PD2)); // UP
  bit_true(DDRD, bit(PD3)); // DN
  bit_true(DDRD, bit(PD4)); // LEFT
  bit_true(DDRD, bit(PD6)); // RIGHT
  bit_true(DDRB, bit(PB7)); // IN
  bit_true(DDRD, bit(PD5)); // OUT
  bit_true(DDRA, bit(PA1)); // BUZZER

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
