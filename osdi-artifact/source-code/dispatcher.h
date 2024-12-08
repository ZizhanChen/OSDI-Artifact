#ifndef __DISPATCHER_H__
#define __DISPATCHER_H__

#include <linux/types.h>
#include "data_queue.h"

void dispatch_once(data_queue* queue);


#endif