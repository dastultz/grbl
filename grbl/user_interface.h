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

// initialize user interface
void ui_init();

// service the user interface (pot, buttons, lights)
void ui_service();

// send a jog command if requested
void ui_handleJogButton();

#endif
