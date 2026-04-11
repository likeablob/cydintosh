#ifndef TOUCH_CYD_H
#define TOUCH_CYD_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include <inttypes.h>

// Touch event structure
typedef struct {
    int16_t x;       // Screen X coordinate (0-319)
    int16_t y;       // Screen Y coordinate (0-239)
    int16_t dx;      // Delta X for mouse
    int16_t dy;      // Delta Y for mouse
    uint8_t pressed; // 1 = touch down, 0 = touch up
    uint8_t clicked; // 1 = click event (transition to pressed)
} touch_event_t;

// Mouse delta structure for queue
typedef struct {
    int16_t dx;
    int16_t dy;
    uint8_t pressed; // Mouse button state: 1=DOWN (double-tap), 0=UP (release)
} mouse_delta_t;

// Double-tap and gesture detection constants
#define DOUBLE_TAP_WINDOW_MS         400 // Time window for double-tap detection
#define TAP_MIN_DURATION_MS          30  // Minimum tap duration to be considered valid
#define TAP_MAX_DURATION_MS          600 // Maximum tap duration for a "tap" (no movement)
#define DOUBLE_TAP_RELEASE_WINDOW_MS 250 // Second tap must occur within this window

// Movement detection
#define DEADZONE_PX           10 // Deadzone for instantaneous movement
#define MOVEMENT_THRESHOLD_PX 15 // Distance from start to be considered "dragging"
#define MOVEMENT_THRESHOLD_SQ (MOVEMENT_THRESHOLD_PX * MOVEMENT_THRESHOLD_PX)

// Initialize touch controller
void touch_init(void);

// Update touch state - call this regularly
// Returns true if touch state changed
int touch_update(void);

// Get current touch event
const touch_event_t *touch_get_event(void);

// Queue-based touch polling interface
void touch_create_queue(void);
QueueHandle_t touch_get_mouse_queue(void);
void touch_start_task(int core, int priority);

#endif