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
#include "system_config.h"

#define LED_OFF				PINS_DRV_ClearPins(PTE, 1<<8)
#define LED_ON				PINS_DRV_SetPins(PTE, 1<<8)
#define LED_TOGGLE			PINS_DRV_TogglePins(PTE, 1<<8)

volatile int exit_code = 0;

const uint8_t message[42] = "No Firmware! Please download a firmware!\r\n";

//uint8_t uart_rx_data;

/* User includes (#include below this line is not maintained by Processor Expert) */

/* Prototypes */
void timer_init(void);
void timer_start(void);
void timer_stop(void);
void timer_interrupt_on(void);
void timer_interrupt_off(void);

//void uart_bluetooth_test(void);

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
    /* SystemInit() has been called in startup_S32K144.s */
//    SystemInit();

    if( STATUS_ERROR == CLOCK_DRV_Init(&clockManager1_InitConfig0) )
    {
    	return exit_code;
    }

    if( STATUS_SUCCESS != PINS_DRV_Init(NUM_OF_CONFIGURED_PINS, g_pin_mux_InitConfigArr) )
    {
    	return exit_code;
    }

//    LPUART_DRV_SendDataPolling(INST_LPUART0, test_text, sizeof(test_text));

    flash_init();

    /*
     * If there is an old firmware, this function will not return.
     * If there is no old firmware, this function will return.
     */
    firmware_update();

    PC2UART_communication_init();

//    firmware_update_test();
//    auto_debug_reset();

//    printf("System is initialized!\n");

//    program_flash_test();
//    eeprom_test();

//    SystemSoftwareReset();

    /*
     * If there is no old firmware, the MCU will run the PC to UART receiver
     * to wait for new firmware downloading from the PC.
     */
    timer_init();
    timer_start();		// Start 200ms timing
	for(;;)
	{
		PC2UART_receiver_run();
		/*
		 * If the firmware is being downloaded, the tasks within the brackets are not executed any more.
		 */
		if( isFirmwareDownloading )
		{
			timer_interrupt_off();
//			PINS_DRV_TogglePins(PTE, 1<<8);
			/*
			 * Now the firmware is not being downloaded,  other tasks can be running.
			 *
			 */
//			PINS_DRV_TogglePins(PTD, 1<<6);
		}
		else
		{
			timer_interrupt_on();
		}
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


//void uart_bluetooth_test(void)
//{
//	status_t uart_status = 0;
//	static uint8_t rxData = 0;
//	uint32_t bytesReceived = 0;
//	uint32_t bytesTransmitted = 0;
////	LPUART_DRV_ReceiveDataPolling(INST_LPUART0, &rxData, 1);
//	uart_status = LPUART_DRV_ReceiveData(INST_LPUART0, &rxData, 1);
//	while( STATUS_BUSY == LPUART_DRV_GetReceiveStatus(INST_LPUART0, &bytesReceived) );
//	uart_status = LPUART_DRV_GetReceiveStatus(INST_LPUART0, &bytesReceived);
//	printf("rx: %c\n", rxData);
//	LPUART_DRV_SendDataPolling(INST_LPUART0, (uint8_t *)"\r\n", 2);	// new line
//
//	LPUART_DRV_SendData(INST_LPUART0, &rxData, 1);
//	while( STATUS_BUSY == LPUART_DRV_GetTransmitStatus(INST_LPUART0, &bytesTransmitted) );
//	uart_status = LPUART_DRV_GetTransmitStatus(INST_LPUART0, &bytesTransmitted);
//	LPUART_DRV_SendDataPolling(INST_LPUART0, (uint8_t *)"\r\n", 2);	// new line
//	PINS_DRV_TogglePins(PTD, 1<<6);
//}

// 200ms timer
void timer_init(void)
{
	LPIT_DRV_Init(INST_LPIT0, &lpit0_InitConfig);
	LPIT_DRV_InitChannel(INST_LPIT0, 0u, &lpit0_ChnConfig0);
	INT_SYS_SetPriority(LPIT0_Ch0_IRQn, INTERRUPT_PRIORITY_LEVEL_TIMER);
	if( 0 == INT_SYS_GetActive(LPIT0_Ch0_IRQn) )
	{
		INT_SYS_ClearPending(LPIT0_Ch0_IRQn);
		INT_SYS_EnableIRQ(LPIT0_Ch0_IRQn);
	}
}

void timer_start(void)
{
	LPIT_DRV_StartTimerChannels(INST_LPIT0, 0x01u);
}

void timer_stop(void)
{
	LPIT_DRV_StopTimerChannels(INST_LPIT0, 0x01u);
}

void timer_interrupt_on(void)
{
	if( !INT_SYS_GetActive(LPIT0_Ch0_IRQn) )
	{
		INT_SYS_ClearPending(LPIT0_Ch0_IRQn);
		INT_SYS_EnableIRQ(LPIT0_Ch0_IRQn);
	}
}

void timer_interrupt_off(void)
{
	if( INT_SYS_GetActive(LPIT0_Ch0_IRQn) )
	{
		INT_SYS_DisableIRQ(LPIT0_Ch0_IRQn);
		INT_SYS_ClearPending(LPIT0_Ch0_IRQn);
	}
}

// 200ms Timing Interrupt
void LPIT0_Ch0_IRQHandler(void)
{
	static uint16_t counter = 0;

	// Clear interrupt flag first
	if( LPIT_DRV_GetInterruptFlagTimerChannels(INST_LPIT0, 0x01u) )
	{
		LPIT_DRV_ClearInterruptFlagTimerChannels(INST_LPIT0, 0x01u);
	}

	if( !isFirmwareDownloading )
	{
		LED_TOGGLE;

		++counter;
		counter %= 5u;
		if(counter == 0)
		{
			// A message is sent out by bluetooth every second.
			LPUART_DRV_SendData(INST_LPUART0, message, sizeof(message));
		}
	}
//	else
//	{
//		/*
//		 * New firmware is being downloaded.
//		 * Increment download time every 200ms for time out check.
//		 */
//		countDownloadTime++;
//	}
}

/*
 * Exception interrupt handlers
 */
void WDOG_EWM_IRQHandler(void)
{
	INT_SYS_ClearPending(WDOG_EWM_IRQn);
	INT_SYS_DisableIRQ(WDOG_EWM_IRQn);
	SystemSoftwareReset();
}

void UsageFault_Handler(void)
{
	INT_SYS_ClearPending(UsageFault_IRQn);
	INT_SYS_DisableIRQ(UsageFault_IRQn);
	SystemSoftwareReset();
}

void SWI_IRQHandler(void)
{
	INT_SYS_ClearPending(SWI_IRQn);
	INT_SYS_DisableIRQ(SWI_IRQn);
	SystemSoftwareReset();
}

void SVC_Handler(void)
{
	INT_SYS_ClearPending(SVCall_IRQn);
	INT_SYS_DisableIRQ(SVCall_IRQn);
	SystemSoftwareReset();
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
