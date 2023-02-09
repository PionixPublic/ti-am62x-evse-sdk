// FIXME (aw): copyright?
#include "FreeRTOS.h"

#include "ti_board_config.h"
#include "ti_board_open_close.h"
#include "ti_drivers_config.h"
#include "ti_drivers_open_close.h"

#include <kernel/dpl/DebugP.h>

#include "bsp_drivers/task_config.h"

void main_task(void* args);

int main() {
    // mandatory system and board initialization
    System_init();
    Board_init();

    Drivers_open();
    Board_driversOpen();

    static StaticTask_t main_task_tcb;
    static StackType_t main_task_stack[MAIN_TASK_STACK_SIZE] __attribute__((aligned(32)));

    // FIXME (aw): task topology?
    auto main_task_handle = xTaskCreateStatic(main_task, "main_task", MAIN_TASK_STACK_SIZE, nullptr, MAIN_TASK_PRIORITY,
                                              main_task_stack, &main_task_tcb);

    configASSERT(main_task_handle != NULL);

    // start the scheduler
    vTaskStartScheduler();

    Board_driversClose();
    Drivers_close();

    // shouldn't be reached, only in case of insufficient FreeRTOS heap memory
    DebugP_assertNoLog(0);

    return 0;
}
