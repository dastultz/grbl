#include "user_interface.h"

// Jog lines to be executed. Zero-terminated.
static char jogLineContinuous[20] = { "$J=G20G91X-100" };
static char jogLineDiscrete[25] = { "$J=G20G91X-0.0005" };
#define AXIS_INDEX 9
#define SIGN_INDEX 10

// index of button mapped to axis properties
char axisMap[] = "ZZXXYY";
char directionMap[] = "+-+-+-";

#define MODE_DISCRETE 0
#define MODE_CONTINUOUS 1
uint8_t mode = MODE_CONTINUOUS;

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

  if (grbl_api_running()) {
    ui_handleOverride();
    ui_handlePause();
  } else if (grbl_api_holding()) {
    ui_handleUnpause();
  } else if (grbl_api_idle() || grbl_api_jogging()) {
    if (mode == MODE_DISCRETE) {
      ui_handleDiscreteJog();
      if (input_edgeHigh(BTN_IDX_TH)) {
        mode = MODE_CONTINUOUS;
        output_clearAllLights();
      }
    } else {
      ui_handleContinuousJogStart();
      ui_handleContinuousJogEnd();
      if (input_edgeHigh(BTN_IDX_TH)) {
        mode = MODE_DISCRETE;
      }
    }
  }

  output_service();
}

void ui_handleOverride() {
  uint8_t pressedButton = input_firstEdgeHighAxisButton();
  if (pressedButton < 6) {
    printString("override\r\n");
  }
}

void ui_handlePause() {
  if (input_edgeHigh(BTN_IDX_TH)) {
    printString("pause\r\n");
  }
}

void ui_handleUnpause() {
  if (input_edgeHigh(BTN_IDX_TH)) {
    printString("unpause\r\n");
  }
}

void ui_handleDiscreteJog() {
  // pair lights with throttle position
  output_exclusiveLightOn(input_throttlePosition());

  // determine if jog button pressed
  ui_axisAction_t action = ui_pickAxisAction();

  if (action.axis) {
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
    jogLineDiscrete[AXIS_INDEX] = action.axis;
    jogLineDiscrete[SIGN_INDEX] = action.direction;
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

    printString(jogLineDiscrete);
    printString("\r\n");
    // todo: if result != 0 start beep cycle
  }
}

void ui_handleContinuousJogStart() {
  // todo: not if any other button pressed
  // determine if jog button pressed
  ui_axisAction_t action = ui_pickAxisAction();

  if (action.axis) {
    printString("continuous start\r\n");
  }
}

void ui_handleContinuousJogEnd() {
  // todo: track the current axis being moved, look for that button
  // to be released
  // determine if jog button released
  ui_axisAction_t action = ui_pickAxisAction();

  if (action.axis) {
    printString("continuous end\r\n");
  }
}

ui_axisAction_t ui_pickAxisAction() {
  ui_axisAction_t action;
  action.axis = 0; // zero if nothing pressed
  action.direction = 0;
  uint8_t pressedButton = input_firstEdgeHighAxisButton();
  if (pressedButton < 6) {
    action.axis = axisMap[pressedButton];
    action.direction = directionMap[pressedButton];
  }
  return action;
}
