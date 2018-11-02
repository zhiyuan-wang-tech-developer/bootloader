/* ###################################################################
**     Filename    : main.c
**     Processor   : S32K14x
**     Abstract    :
**         Main module.
**         This module contains user's application code.
**     Settings    :
**     Contents    :
**         No public methods
**
** ###################################################################*/
/*!
** @file main.c
** @version 01.00
** @brief
**         Main module.
**         This module contains user's application code.
*/
/*!
**  @addtogroup main_module main module documentation
**  @{
*/
/* MODULE main */


/* Including necessary module. Cpu.h contains other modules needed for compiling.*/
#include "Cpu.h"
#include "pc_communication.h"
#include "bootloader.h"
#include "stdio.h"
#include "string.h"

volatile int exit_code = 0;

uint8_t delete_text[16] = {0xFF};

uint8_t uart_rx_data;


/* User includes (#include below this line is not maintained by Processor Expert) */

/* Prototypes */
void uart_bluetooth_test(void);

/*!
  \brief The main function for the project.
  \details The startup initialization sequence is the following:
 * - startup asm routine
 * - main()
*/
int main(void)
{
  /* Write your local variable definition here */

  /*** Processor Expert internal initialization. DON'T REMOVE THIS CODE!!! ***/
  #ifdef PEX_RTOS_INIT
    PEX_RTOS_INIT();                   /* Initialization of the selected RTOS. Macro is defined by the RTOS component. */
  #endif
  /*** End of Processor Expert internal initialization.                    ***/

  /* Write your code here */
  /* For example: for(;;) { } */
    SystemInit();

    if( STATUS_ERROR == CLOCK_DRV_Init(&clockManager1_InitConfig0) )
    {
    	return exit_code;
    }

    if( STATUS_SUCCESS != PINS_DRV_Init(NUM_OF_CONFIGURED_PINS, g_pin_mux_InitConfigArr) )
    {
    	return exit_code;
    }

    PC2UART_communication_init();

//    LPUART_DRV_SendDataPolling(INST_LPUART0, test_text, sizeof(test_text));

    flash_init();
    firmware_update();
//    firmware_update_test();
//    auto_debug_reset();

//    printf("System is initialized!\n");

//    program_flash_test();
//    eeprom_test();

//    SystemSoftwareReset();


	for(;;)
	{
		PC2UART_receiver_run_test();
	//    	uart_bluetooth_test();
	}

	/*** Don't write any code pass this line, or it will be deleted during code generation. ***/
  /*** RTOS startup code. Macro PEX_RTOS_START is defined by the RTOS component. DON'T MODIFY THIS CODE!!! ***/
  #ifdef PEX_RTOS_START
    PEX_RTOS_START();                  /* Startup of the selected RTOS. Macro is defined by the RTOS component. */
  #endif
  /*** End of RTOS startup code.  ***/
  /*** Processor Expert end of main routine. DON'T MODIFY THIS CODE!!! ***/
  for(;;) {
    if(exit_code != 0) {
      break;
    }
  }
  return exit_code;
  /*** Processor Expert end of main routine. DON'T WRITE CODE BELOW!!! ***/
} /*** End of main routine. DO NOT MODIFY THIS TEXT!!! ***/


void uart_bluetooth_test(void)
{
	status_t uart_status = 0;
	static uint8_t rxData = 0;
	uint32_t bytesReceived = 0;
	uint32_t bytesTransmitted = 0;
//	LPUART_DRV_ReceiveDataPolling(INST_LPUART0, &rxData, 1);
	uart_status = LPUART_DRV_ReceiveData(INST_LPUART0, &rxData, 1);
	while( STATUS_BUSY == LPUART_DRV_GetReceiveStatus(INST_LPUART0, &bytesReceived) );
	uart_status = LPUART_DRV_GetReceiveStatus(INST_LPUART0, &bytesReceived);
	printf("rx: %c\n", rxData);
	LPUART_DRV_SendDataPolling(INST_LPUART0, (uint8_t *)"\r\n", 2);	// new line

	LPUART_DRV_SendData(INST_LPUART0, &rxData, 1);
	while( STATUS_BUSY == LPUART_DRV_GetTransmitStatus(INST_LPUART0, &bytesTransmitted) );
	uart_status = LPUART_DRV_GetTransmitStatus(INST_LPUART0, &bytesTransmitted);
	LPUART_DRV_SendDataPolling(INST_LPUART0, (uint8_t *)"\r\n", 2);	// new line
	PINS_DRV_TogglePins(PTD, 1<<6);
}


/* END main */
/*!
** @}
*/
/*
** ###################################################################
**
**     This file was created by Processor Expert 10.1 [05.21]
**     for the Freescale S32K series of microcontrollers.
**
** ###################################################################
*/
