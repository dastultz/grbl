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

  // in discrete jog mode, pair lights with throttle position
  if (input_throttlePositionChanged()) {
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
    // must capture in local variable first, if we don't
    // the second of a double tap doesn't match switch statement
    // don't know why
    uint8_t tp = input_throttlePosition();
    switch (tp) {
      case 0: zeros = 3; num = '5'; break; // 0.0005
      case 1: zeros = 2; num = '1'; break; // 0.001
      case 2: zeros = 2; num = '5'; break; // 0.005
      case 3: zeros = 1; num = '1'; break; // 0.010
      case 4: zeros = 1; num = '2'; break; // 0.020
      case 5: zeros = 0; num = '1'; break; // 0.100
    }
    if (input_edgeHigh(BTN_IDX_UP)) {
      axis = 'Z';
    } else if (input_edgeHigh(BTN_IDX_DN)) {
      axis = 'Z';
      direction = '-';
    } else if (input_edgeHigh(BTN_IDX_LF)) {
      axis = 'X';
      direction = '-';
    } else if (input_edgeHigh(BTN_IDX_RT)) {
      axis = 'X';
    } else if (input_edgeHigh(BTN_IDX_IN)) {
      axis = 'Y';
    } else if (input_edgeHigh(BTN_IDX_OT)) {
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

      uint8_t result = grbl_api_executeJog(jogLineDiscrete);
      // todo: if result != 0 start beep cycle
    }
  }
}
