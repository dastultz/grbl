#include "user_interface.h"

// G-Code fragments
static char JOG[] = "$J=";
static char G20[] = "G20";
static char G21[] = "G21";
static char G91[] = "G91";

#define JOG_LENGTH 25
static char jogCommand[JOG_LENGTH];

// index of button mapped to axis properties
static char axisMap[] = "ZZXXYY";
static char directionMap[] = "+--++-";

#define MODE_DISCRETE 0
#define MODE_CONTINUOUS 1
uint8_t mode = MODE_CONTINUOUS;

uint8_t activeContinuousButton = 0;

unsigned long loopCounter;

void ui_init() {
  input_init();
  output_init();
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
    } else { // MODE_CONTINUOUS
      if (grbl_api_jogging()) {
        if (! grbl_api_planner_full()) {
          ui_sendContinuousJog();
        }
        ui_handleContinuousJogEnd();
      } else {
        ui_handleContinuousJogStart();
        if (input_edgeHigh(BTN_IDX_TH)) {
          mode = MODE_DISCRETE;
        }
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

  ui_initAxisAction();

  if (activeContinuousButton < 6) {
    ui_clearJogCommand();
    strcat(jogCommand, JOG);
    strcat(jogCommand, G20); // inch
    strcat(jogCommand, G91); // relative
    jogCommand[strlen(jogCommand)] = axisMap[activeContinuousButton];
    jogCommand[strlen(jogCommand)] = directionMap[activeContinuousButton];
    strcat(jogCommand, "0.");
    // must capture in local variable first, if we don't
    // the second of a double tap doesn't match switch statement
    // don't know why
    uint8_t tp = input_throttlePosition();
    switch (tp) {
      case 0: strcat(jogCommand, "0005"); break;
      case 1: strcat(jogCommand, "001"); break;
      case 2: strcat(jogCommand, "005"); break;
      case 3: strcat(jogCommand, "01"); break;
      case 4: strcat(jogCommand, "02"); break;
      case 5: strcat(jogCommand, "1"); break;
    }
    // Feed of 10 ipm
    strcat(jogCommand, "F10");

    uint8_t result = grbl_api_executeJog(jogCommand);

    printString(jogCommand);
    printString(":");
    print_uint8_base10(result);
    printString("\r\n");
    // todo: if result != 0 start beep cycle
  }
}

void ui_handleContinuousJogStart() {
  ui_initAxisAction();

  if (activeContinuousButton < 6) {
    ui_sendContinuousJog();
    printString("continuous start\r\n");
  }
}

void ui_handleContinuousJogEnd() {
  if (input_edgeLow(activeContinuousButton)) {
    grbl_api_cancelJog();
    printString("continuous end\r\n");
  }
}

void ui_sendContinuousJog() {
  ui_clearJogCommand();
  strcat(jogCommand, JOG);
  strcat(jogCommand, G21); // metric
  strcat(jogCommand, G91); // relative
  jogCommand[strlen(jogCommand)] = axisMap[activeContinuousButton];
  jogCommand[strlen(jogCommand)] = directionMap[activeContinuousButton];
  strcat(jogCommand, "0.0");
  // must capture in local variable first, if we don't
  // the second of a double tap doesn't match switch statement
  // don't know why
  uint8_t tp = input_throttlePosition();
  // these numbers are fractions of a millimeter per move!
  switch (tp) {
    case 0: strcat(jogCommand, "056"); break;
    case 1: strcat(jogCommand, "112"); break;
    case 2: strcat(jogCommand, "282"); break;
    case 3: strcat(jogCommand, "423"); break;
    case 4: strcat(jogCommand, "564"); break;
    case 5: strcat(jogCommand, "705"); break;
  }
  strcat(jogCommand, "F");
  // these numbers are millimeters per minute!
  switch (tp) {
    case 0: strcat(jogCommand, "50"); break;
    case 1: strcat(jogCommand, "101"); break;
    case 2: strcat(jogCommand, "254"); break;
    case 3: strcat(jogCommand, "381"); break;
    case 4: strcat(jogCommand, "508"); break;
    case 5: strcat(jogCommand, "635"); break;
  }
  uint8_t result = grbl_api_executeJog(jogCommand);

  if (loopCounter % 3000 == 0) {
    printString(jogCommand);
    printString(":");
    print_uint8_base10(result);
    printString("\r\n");
  }
  // todo: if result != 0 start beep cycle
}

void ui_initAxisAction() {
  uint8_t pressedButton = input_firstEdgeHighAxisButton();
  if (pressedButton < 6) {
    activeContinuousButton = pressedButton;
  } else {
    activeContinuousButton = 6;
  }
}

void ui_clearJogCommand() {
  for (uint8_t i = 0; i < JOG_LENGTH; i++) {
    jogCommand[i] = 0;
  }
}
