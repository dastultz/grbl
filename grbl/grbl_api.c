#include "grbl_api.h"

void grbl_api_e_stop() {
  mc_reset();
}

void grbl_clear_alarm() {

}

void grbl_api_feed_override_reset() {
  system_set_exec_motion_override_flag(EXEC_FEED_OVR_RESET);
}

void grbl_api_feed_override_coarse_plus() {
  system_set_exec_motion_override_flag(EXEC_FEED_OVR_COARSE_PLUS);
}

void grbl_api_feed_override_coarse_minus() {
  system_set_exec_motion_override_flag(EXEC_FEED_OVR_COARSE_MINUS);
}

void grbl_api_feed_override_fine_plus() {
  system_set_exec_motion_override_flag(EXEC_FEED_OVR_FINE_PLUS);
}

void grbl_api_feed_override_fine_minus() {
  system_set_exec_motion_override_flag(EXEC_FEED_OVR_FINE_MINUS);
}

void grbl_api_rapid_override_reset() {
  system_set_exec_motion_override_flag(EXEC_RAPID_OVR_RESET);
}

void grbl_api_rapid_override_medium() {
  system_set_exec_motion_override_flag(EXEC_RAPID_OVR_MEDIUM);
}

void grbl_api_rapid_override_low() {
  system_set_exec_motion_override_flag(EXEC_RAPID_OVR_LOW);
}

void grbl_api_pause() {
  system_set_exec_motion_override_flag(EXEC_FEED_HOLD);
}

void grbl_api_unpause() {
  system_set_exec_motion_override_flag(EXEC_CYCLE_START);
}

int grbl_api_idle() {
  return sys.state == STATE_IDLE;
}

int grbl_api_jogging() {
  return sys.state == STATE_JOG;
}

int grbl_api_running() {
  return sys.state == STATE_CYCLE;
}

int grbl_api_holding() {
  return sys.state == STATE_HOLD;
}

int grbl_api_planner_full() {
  return plan_check_full_buffer();
}

uint8_t grbl_api_executeJog(char *line) {
  return system_execute_line(line);
}

void grbl_api_cancelJog() {
  if (sys.state == STATE_JOG) {
    system_set_exec_state_flag(EXEC_MOTION_CANCEL);
  }
}



