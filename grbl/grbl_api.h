#ifndef grbl_api_h
#define grbl_api_h

#include "grbl.h"

/*
Functions for interfacing with GRBL
*/
void grbl_api_e_stop();

void grbl_api_feed_override_reset();

void grbl_api_feed_override_coarse_plus();

void grbl_api_feed_override_coarse_minus();

void grbl_api_feed_override_fine_plus();

void grbl_api_feed_override_fine_minus();

void grbl_api_rapid_override_reset();

void grbl_api_rapid_override_medium();

void grbl_api_rapid_override_low();

void grbl_api_pause();

void grbl_api_unpause();

int grbl_api_idle();

int grbl_api_jogging();

int grbl_api_running();

int grbl_api_holding();

int grbl_api_planner_full();

uint8_t grbl_api_executeJog();

void grbl_api_cancelJog();

#endif
