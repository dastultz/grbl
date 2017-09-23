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

void ui_init() {
  input_init();
  output_init();
  ui_watchDogInit();
}

void ui_watchDogInit() {
  // Clear reset, we can not clear WDE if WDRF is set
  MCUSR &= ~(1<<WDRF);
  cli(); // disable interrupts so we do not get interrupted while doing timed sequence
  // First step of timed sequence, we have 4 cycles after this to make changes to WDE and WD timeout
  WDTCSR |= (1<<WDCE) | (1<<WDE);
  // disable reset mode and set prescaler in one operation
  WDTCSR = 0; // 15ms timeout, about 66Hz
  sei(); // enable global interrupts
  WDTCSR |= _BV(WDIE); // enable watchdog interrupt only mode
}

// when running a cycle, this handles I/O
ISR(WDT_vect) { // Watchdog timer ISR
  if (grbl_api_running() || grbl_api_holding()) {
    input_service();
    ui_handleEStop();
    ui_handleSpindleThermalShutdown();
    if (grbl_api_running()) {
      ui_handleOverride();
      ui_handlePause();
    } else if (grbl_api_holding()) {
      ui_handleUnpause();
    }
    output_clearAllLights();
    output_service();
  }
}

void ui_service() {
  if (grbl_api_idle() || grbl_api_jogging()) {
    input_service();
    ui_handleSpindleThermalShutdown();
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
    output_service();
  }
}

void ui_handleEStop() {
  if (input_edgeHigh(BTN_IDX_ES)) {
    mc_reset(); // todo: move this out to grbl_api
  }
  if (input_edgeLow(BTN_IDX_ES)) {
    grbl_clear_alarm();
  }
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

    // todo: if result != 0 start beep cycle
  }
}

void ui_handleContinuousJogStart() {
  ui_initAxisAction();

  if (activeContinuousButton < 6) {
    ui_sendContinuousJog();
  }
}

void ui_handleContinuousJogEnd() {
  if (input_edgeLow(activeContinuousButton)) {
    grbl_api_cancelJog();
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
    case 3: strcat(jogCommand, "304"); break;
    case 4: strcat(jogCommand, "381"); break;
    case 5: strcat(jogCommand, "457"); break;
  }
  uint8_t result = grbl_api_executeJog(jogCommand);

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
