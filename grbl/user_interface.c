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

int8_t activeJogButton = -1;

void ui_init() {
  input_init();
  output_init();
}

// this is an interrupt routine
// "edge" test on digital inputs will work in this "thread"
void ui_handleInputChange() {

  input_readDigital();
  ui_handleSpindleThermalShutdown();

  if (grbl_api_running()) {
    ui_handleOverride();
    ui_handlePause();
  } else if (grbl_api_holding()) {
    ui_handleUnpause();
  }

  if (grbl_api_idle() || grbl_api_jogging()) {
    if (mode == MODE_DISCRETE) {
      ui_initJogAction(); // possibly discrete jog start
      if (grbl_api_idle() && input_edgeHigh(BTN_IDX_TH)) {
        mode = MODE_CONTINUOUS;
      }
    } else { // MODE_CONTINUOUS
      ui_handleContinuousJogStart();
      ui_handleContinuousJogEnd();
      if (grbl_api_idle() && input_edgeHigh(BTN_IDX_TH)) {
        mode = MODE_DISCRETE;
      }
    }
  }
  output_service();
  input_clear();
}

// this is called from the main loop
// "edge" tests on digital inputs don't work in this "thread"!
void ui_service() {
  if (grbl_api_running() || mode == MODE_CONTINUOUS) {
    output_clearAllLights();
  }
  if (grbl_api_idle() || grbl_api_jogging()) {
    input_readAnalog();
    if (mode == MODE_DISCRETE) {
      // pair lights with throttle position
      output_exclusiveLightOn(input_throttlePosition());
    }
    if (activeJogButton > -1) {
      if (mode == MODE_DISCRETE) {
        ui_handleDiscreteJog();
      } else if (! grbl_api_planner_full()) { // CONTINUOUS
        ui_sendContinuousJog();
      }
    }
  }
  output_service();
}

void ui_handleSpindleThermalShutdown() {
  if (input_edgeHigh(BTN_IDX_SP)) output_setHigh(OUT_IDX_BZ);
  if (input_edgeLow(BTN_IDX_SP)) output_setLow(OUT_IDX_BZ);
}

void ui_handleOverride() {
  uint8_t pressedButton = input_firstEdgeHighAxisButton();
  switch (pressedButton) {
    case BTN_IDX_UP: grbl_api_feed_override_coarse_plus(); break;
    case BTN_IDX_DN: grbl_api_feed_override_coarse_minus(); break;
    case BTN_IDX_IN: grbl_api_feed_override_fine_plus(); break;
    case BTN_IDX_OT: grbl_api_feed_override_fine_minus(); break;
    case BTN_IDX_LF: grbl_api_rapid_override_minus(); break;
    case BTN_IDX_RT: grbl_api_rapid_override_plus(); break;
  }
}

void ui_handlePause() {
  if (input_edgeHigh(BTN_IDX_TH)) {
    grbl_api_pause();
  }
}

void ui_handleUnpause() {
  if (input_edgeHigh(BTN_IDX_TH)) {
    grbl_api_unpause();
  }
}

void ui_handleDiscreteJog() {
  // capture current jog button to protect from interrupt routine changing it
  int8_t jogButton = activeJogButton;
  if (jogButton == -1) return;

  ui_clearJogCommand();
  strcat(jogCommand, JOG);
  strcat(jogCommand, G20); // inch
  strcat(jogCommand, G91); // relative
  jogCommand[strlen(jogCommand)] = axisMap[jogButton];
  jogCommand[strlen(jogCommand)] = directionMap[jogButton];
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
  // clear active jog button
  activeJogButton = -1;
}

void ui_handleContinuousJogStart() {
  ui_initJogAction();
}

void ui_handleContinuousJogEnd() {
  if (activeJogButton > -1 && input_edgeLow(activeJogButton)) {
    if (grbl_api_jogging()) grbl_api_cancelJog();
    activeJogButton = -1;
  }
}

void ui_sendContinuousJog() {
  // capture current jog button to protect from interrupt routine changing it
  int8_t jogButton = activeJogButton;
  if (jogButton == -1) return;

  ui_clearJogCommand();
  strcat(jogCommand, JOG);
  strcat(jogCommand, G21); // metric
  strcat(jogCommand, G91); // relative
  jogCommand[strlen(jogCommand)] = axisMap[jogButton];
  jogCommand[strlen(jogCommand)] = directionMap[jogButton];
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
    case 3: strcat(jogCommand, "304"); break;
    case 4: strcat(jogCommand, "381"); break;
    case 5: strcat(jogCommand, "457"); break;
  }
  uint8_t result = grbl_api_executeJog(jogCommand);
}

void ui_initJogAction() {
  activeJogButton = input_firstEdgeHighAxisButton();
}

void ui_clearJogCommand() {
  for (uint8_t i = 0; i < JOG_LENGTH; i++) {
    jogCommand[i] = 0;
  }
}
