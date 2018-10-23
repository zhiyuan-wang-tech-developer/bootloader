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
#include "stdio.h"

volatile int exit_code = 0;

uint8_t test_text[] = "allround technology autoliv test\n";

flash_ssd_config_t flashSSDConfig;

/* User includes (#include below this line is not maintained by Processor Expert) */

/* Prototypes */
void uart_bluetooth_test(void);
void flash_test(void);

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

    LPUART_DRV_Init(INST_LPUART0, &lpuart0_State, &lpuart0_InitConfig0);
    FLASH_DRV_Init(&Flash_InitConfig0, &flashSSDConfig);

    LPUART_DRV_SendDataPolling(INST_LPUART0, test_text, sizeof(test_text));

//    printf("System is initialized!\n");

    flash_test();


    for(;;)
    {
    	uart_bluetooth_test();
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
	static	uint8_t rxData = 0;
//        LPUART_DRV_SendDataPolling(INST_LPUART0, test_text, sizeof(test_text));
	LPUART_DRV_ReceiveDataPolling(INST_LPUART0, &rxData, 1);
//        LPUART_Getchar(LPUART0_BASE, &rxData);
	printf("rx: %c\n", rxData);
	LPUART_DRV_SendDataPolling(INST_LPUART0, &rxData, 1);
//    	LPUART_DRV_SendDataPolling(INST_LPUART0, (uint8_t *)'\n', 1);
	PINS_DRV_TogglePins(PTD, 1<<6);
}


void flash_test(void)
{
	uint32_t flash_program_address_1 = 0x0005F090;
	uint32_t flash_program_address_2 = flash_program_address_1 + 32;
	uint32_t flash_program_sector_address = 0;
	uint32_t flash_protect_status = 0;
	status_t flash_status = 0;
//	uint32_t checksum = 0;
	uint32_t failaddress = 0;
	uint8_t i = 0;
	uint8_t readDataArray1[32] = {0};
	uint8_t readDataArray2[32] = {0};
	FLASH_DRV_GetPFlashProtection(&flash_protect_status);
	flash_program_sector_address = flash_program_address_1 - (flash_program_address_1 % FEATURE_FLS_PF_BLOCK_SECTOR_SIZE);
	flash_status = FLASH_DRV_EraseSector(&flashSSDConfig, flash_program_sector_address, FEATURE_FLS_PF_BLOCK_SECTOR_SIZE);
//	flash_status = FLASH_DRV_Program(&flashSSDConfig, flash_program_address, sizeof(test_text), test_text);
//	flash_status = FLASH_DRV_ProgramCheck(&flashSSDConfig, flash_program_address, sizeof(test_text), test_text, &failaddress, 0x01);
	flash_status = FLASH_DRV_Program(&flashSSDConfig, flash_program_address_1, 16, test_text);
	flash_status = FLASH_DRV_ProgramCheck(&flashSSDConfig, flash_program_address_1, 16, test_text, &failaddress, 0x01);
//	flash_status = FLASH_DRV_CheckSum(&flashSSDConfig, flash_program_address, sizeof(test_text), &checksum);

	for( i = 0; i < 16; i++ )
	{
		readDataArray1[i] = *((uint8_t *)flash_program_address_1 + i);
	}

//	flash_status = FLASH_DRV_Program(&flashSSDConfig, flash_program_address2, sizeof(test_text), test_text);
//	flash_status = FLASH_DRV_ProgramCheck(&flashSSDConfig, flash_program_address2, sizeof(test_text), test_text, &failaddress, 0x01);
	flash_status = FLASH_DRV_Program(&flashSSDConfig, flash_program_address_2, 32, test_text);
	flash_status = FLASH_DRV_ProgramCheck(&flashSSDConfig, flash_program_address_2, 32, test_text, &failaddress, 0x01);
	for( i = 0; i < 32; i++ )
	{
		readDataArray2[i] = *((uint8_t *)flash_program_address_2 + i);
	}
	flash_status = FLASH_DRV_EraseSector(&flashSSDConfig, flash_program_sector_address, 4096u);
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
