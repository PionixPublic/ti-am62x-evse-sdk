#ifndef DRIVERS_TASK_CONFIG_H
#define DRIVERS_TASK_CONFIG_H

#include "task.h"

// FIXME (aw): use proper task priorities and stack sizes
#define MAIN_TASK_PRIORITY   (configMAX_PRIORITIES - 1)
#define MAIN_TASK_STACK_SIZE (16384U / sizeof(configSTACK_DEPTH_TYPE))

#define SAMPLING_TASK_PRIORITY   (configMAX_PRIORITIES - 2)
#define SAMPLING_TASK_STACK_SIZE (1024U / sizeof(configSTACK_DEPTH_TYPE))

#endif // DRIVERS_TASK_CONFIG_H
