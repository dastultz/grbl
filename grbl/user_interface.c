#include "user_interface.h"

// Jog lines to be executed. Zero-terminated.
static char jogLineContinuous[20] = { "$J=G20G91X-100" };
static char jogLineDiscrete[25] = { "$J=G20G91X-0.0005" };
#define AXIS_INDEX 9
#define SIGN_INDEX 10

unsigned long loopCounter;

void ui_init() {
  input_init();
  output_init();
  jogLineContinuous[14] = 0;
  jogLineDiscrete[16] = 0;
}

void ui_service() {
  loopCounter++;
  input_service();

  for (uint8_t i = 0; i < 10; i++) {
    if (input_edgeHigh(i)) {
      printString("HIGH ");
      print_uint8_base10(i);
      printString(" ");
      printInteger(input_edgeHigh(i));
      printString("\r\n");
      if (i < 7) output_setHigh(i);
    }
    if (input_edgeLow(i)) {
      printString("LOW ");
      print_uint8_base10(i);
      printString(" ");
      printInteger(input_edgeLow(i));
      printString("\r\n");
      if (i < 7) output_setLow(i);
    }
  }
  if (input_throttlePositionChanged()) {
    printString("TH ");
    print_uint8_base10(input_throttlePosition());
    printString("\r\n");
    for (uint8_t i = 0; i < 6; i++) {
      output_setLow(i);
    }
    output_setHigh(input_throttlePosition());
  }

  output_service();

  ui_handleJogButton();
}

void ui_handleJogButton() {
  if (grbl_api_idle() || grbl_api_jogging()) {
    char direction = '+';
    char axis = 0;
    // define distance in ten-thousandths of an inch with
    // single-digit number and leading zeros
    uint8_t zeros = 0; // leading zeros
    char num = 0;
    switch (5) { //input_throttlePosition()) {
      case 0: zeros = 3; num = '5'; break; // 0.0005
      case 1: zeros = 2; num = '1'; break; // 0.001
      case 2: zeros = 2; num = '5'; break; // 0.005
      case 3: zeros = 1; num = '1'; break; // 0.010
      case 4: zeros = 1; num = '2'; break; // 0.020
      case 5: zeros = 0; num = '1'; break; // 0.100
    }
    // todo: do we need to handle button exclusivity?
    if (input_edgeLow(BTN_IDX_UP)) {
      axis = 'Z';
    } else if (input_edgeLow(BTN_IDX_DN)) {
      axis = 'Z';
      direction = '-';
    } else if (input_edgeLow(BTN_IDX_LF)) {
      axis = 'X';
      direction = '-';
    } else if (input_edgeLow(BTN_IDX_RT)) {
      axis = 'X';
    } else if (input_edgeLow(BTN_IDX_IN)) {
      axis = 'Y';
    } else if (input_edgeLow(BTN_IDX_OT)) {
      axis = 'Y';
      direction = '-';
    }
    if (axis) {
      jogLineDiscrete[SIGN_INDEX] = direction;
      jogLineDiscrete[AXIS_INDEX] = axis;
      uint8_t index = SIGN_INDEX + 1;
      jogLineDiscrete[index] = '0'; index++;
      jogLineDiscrete[index] = '.'; index++;
      for (uint8_t i = 0; i < zeros; i++) {
        jogLineDiscrete[index] = '0';
        index++;
      }
      jogLineDiscrete[index] = num; index++;

      // Feed of 10 ipm
      jogLineDiscrete[index] = 'F'; index++;
      jogLineDiscrete[index] = '1'; index++;
      jogLineDiscrete[index] = '0'; index++;

      jogLineDiscrete[index] = 0; // zero terminated
      printString(jogLineDiscrete);
      printString("\r\n");

      uint8_t result = grbl_api_executeJog(jogLineDiscrete);

      printString("J result ");
      print_uint8_base10(result);
      printString("\r\n");
    }
  }
}
