#ifndef DISC_LFS_H
#define DISC_LFS_H

#include "disc.h"

int disc_lfs_init(void);
void disc_lfs_deinit(void);
int disc_lfs_open(disc_descr_t *disc, const char *filename, int read_only);
void disc_lfs_close(disc_descr_t *disc);

#endif