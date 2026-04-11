#ifndef DISPLAY_H
#define DISPLAY_H

#include <inttypes.h>

void display_init(void);
void display_task_start(int core, int priority);
void display_notify_update(void);
void display_set_framebuffer(const uint8_t *fb);

#endif