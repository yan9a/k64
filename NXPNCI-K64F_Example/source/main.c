/*
*         Copyright (c), NXP Semiconductors Caen / France
*
*                     (C)NXP Semiconductors
*       All rights are reserved. Reproduction in whole or in part is
*      prohibited without the written consent of the copyright owner.
*  NXP reserves the right to make changes without notice at any time.
* NXP makes no warranty, expressed, implied or statutory, including but
* not limited to any implied warranty of merchantability or fitness for any
*particular purpose, or that the use will not infringe any third party patent,
* copyright or trademark. NXP must not be liable for any loss or damage
*                          arising from its use.
*/

#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "board.h"
#include "pin_mux.h"
#include "clock_config.h"
#include <nfc_task.h>
#include "fsl_debug_console.h"

//-----------------------------------------------------------------------
// Macros
//-----------------------------------------------------------------------
#define TASK_NFC_STACK_SIZE		1024
#define TASK_NFC_STACK_PRIO		(configMAX_PRIORITIES - 1)

int main(void) {
    /* Init board hardware. */
    BOARD_InitPins();
    BOARD_BootClockRUN();
    BOARD_InitDebugConsole();

	PRINTF("\nRunning the NXP-NCI project.\n");

	/* Create NFC task */
    if (xTaskCreate((TaskFunction_t) task_nfc,
    				(const char*) "NFC_task",
					TASK_NFC_STACK_SIZE,
					NULL,
					TASK_NFC_STACK_PRIO,
					NULL) != pdPASS)
    {
    	PRINTF("Failed to create NFC task");
    }

    vTaskStartScheduler();

    do {} while(1);

	return 0;
}

////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
