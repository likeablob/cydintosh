#ifndef VIDEO_H
#define VIDEO_H

#include <inttypes.h>

void video_init(uint32_t *framebuffer);
void video_update(void);

#endif