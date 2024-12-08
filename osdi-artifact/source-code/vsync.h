#ifndef __VSYNC_H__
#define __VSYNC_H__

#include <linux/types.h>
#include <linux/fs.h>
#include "data_queue.h"

//void vsync_signal();
int drm_init(void);
void dispatch_vsync(void);

#endif