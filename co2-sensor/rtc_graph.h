// rtc_graph.h — RTC memory for CO2 history across deep sleep cycles
// RTC_DATA_ATTR survives deep sleep, resets only on full power loss
#pragma once
#include <string.h>

// 60 points = always 1h window
// (60pts @ 1min, 30pts @ 2min, 12pts @ 5min visible on graph)
#define RTC_GRAPH_POINTS 60
#define RTC_MAGIC_VAL 0xC02A1234u

RTC_DATA_ATTR uint32_t rtc_magic      = 0;
RTC_DATA_ATTR float    rtc_co2_history[RTC_GRAPH_POINTS];
RTC_DATA_ATTR uint16_t rtc_step_history[RTC_GRAPH_POINTS]; // interval in seconds when reading was taken
RTC_DATA_ATTR int   rtc_write_idx   = 0;
RTC_DATA_ATTR int   rtc_count       = 0;
RTC_DATA_ATTR int   rtc_boot_count  = 0;

// Battery state persists across deep sleep
RTC_DATA_ATTR int   rtc_batt_pct    = 0;
RTC_DATA_ATTR float rtc_last_batt_v = 0.0f;
RTC_DATA_ATTR bool  rtc_is_charging = false;

// Diagnostic: last detected wipe event.
// Set in on_boot when magic mismatch indicates RTC was wiped. Survives
// subsequent wakes (magic is OK again) so the cause is retrievable later
// via WiFi-on log session — no flash wear required.
RTC_DATA_ATTR uint8_t  rtc_last_wipe_reason = 0;   // esp_reset_reason_t of last wipe
RTC_DATA_ATTR uint8_t  rtc_last_wipe_wakeup = 0;   // esp_sleep_wakeup_cause_t of last wipe
RTC_DATA_ATTR uint32_t rtc_last_wipe_boot  = 0;    // rtc_boot_count value just before last wipe

// Call once in on_boot — detects garbage RTC after new firmware or power cycle
inline void rtc_validate() {
    if (rtc_magic != RTC_MAGIC_VAL) {
        rtc_magic      = RTC_MAGIC_VAL;
        rtc_write_idx  = 0;
        rtc_count      = 0;
        rtc_boot_count = 0;
        rtc_batt_pct   = 0;
        rtc_last_batt_v = 0.0f;
        rtc_is_charging = false;
        memset(rtc_co2_history, 0, sizeof(rtc_co2_history));
        memset(rtc_step_history, 0, sizeof(rtc_step_history));
    }
}

void rtc_push_co2(float val, uint16_t step_secs) {
    // Guard: never trust index after firmware change
    if (rtc_write_idx < 0 || rtc_write_idx >= RTC_GRAPH_POINTS) rtc_write_idx = 0;
    if (rtc_count < 0 || rtc_count > RTC_GRAPH_POINTS) rtc_count = 0;
    rtc_co2_history[rtc_write_idx] = val;
    rtc_step_history[rtc_write_idx] = step_secs;
    rtc_write_idx = (rtc_write_idx + 1) % RTC_GRAPH_POINTS;
    if (rtc_count < RTC_GRAPH_POINTS) rtc_count++;
}

// age=0 → newest reading, age=rtc_count-1 → oldest
float rtc_get_co2_at(int age) {
    if (age >= rtc_count || rtc_count == 0) return 0.0f;
    int idx = ((rtc_write_idx - 1 - age) % RTC_GRAPH_POINTS + RTC_GRAPH_POINTS) % RTC_GRAPH_POINTS;
    return rtc_co2_history[idx];
}

// Get the interval (seconds) that was active when this reading was taken
uint16_t rtc_get_step_at(int age) {
    if (age >= rtc_count || rtc_count == 0) return 60;
    int idx = ((rtc_write_idx - 1 - age) % RTC_GRAPH_POINTS + RTC_GRAPH_POINTS) % RTC_GRAPH_POINTS;
    return rtc_step_history[idx] > 0 ? rtc_step_history[idx] : 60;
}
