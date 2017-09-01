#ifndef user_interface_h
#define user_interface_h

#include "grbl.h"

// index into array of digital inputs
#define BTN_IDX_UP 0
#define BTN_IDX_DN 1
#define BTN_IDX_LF 2
#define BTN_IDX_RT 3
#define BTN_IDX_IN 4
#define BTN_IDX_OT 5
#define BTN_IDX_TH 6
#define BTN_IDX_ES 7
#define BTN_IDX_LM 8
#define BTN_IDX_SP 9

// index into array of digital outputs
#define OUT_IDX_UP 0
#define OUT_IDX_DN 1
#define OUT_IDX_LF 2
#define OUT_IDX_RT 3
#define OUT_IDX_IN 4
#define OUT_IDX_OT 5
#define OUT_IDX_BZ 6

// initialize user interface
void ui_init();

// service the user interface (pot, buttons, lights)
void ui_service();

// initialize watchdog timer used for servicing inputs during cycle
void ui_watchDogInit();

void ui_handleEStop();

void ui_handleSpindleThermalShutdown();

// null out the buffer before building new line
void ui_clearJogCommand();

// issue feed/rapid override if requested
void ui_handleOverride();

// issue a feed hold if requested
void ui_handlePause();

// issue a resume if requested
void ui_handleUnpause();

// issue a discrete jog command if requested
void ui_handleDiscreteJog();

// issue a continuous jog command if requested
void ui_handleContinuousJogStart();

// cancel a continuous jog command if requested
void ui_handleContinuousJogEnd();

// axis button action taken
void ui_initAxisAction();

// send continuous jog move
void ui_sendContinuousJog();

#endif
