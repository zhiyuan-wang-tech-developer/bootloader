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
#include "stdio.h"
#include "string.h"

volatile int exit_code = 0;

uint8_t test_text[] = "allround technology autoliv test\r\n";
uint8_t delete_text[16] = {0xFF};

uint8_t uart_rx_data;


flash_ssd_config_t flashSSDConfig;

/* User includes (#include below this line is not maintained by Processor Expert) */

/* Prototypes */
void uart_bluetooth_test(void);
void flash_test_init(void);
void program_flash_test(void);
void eeprom_test(void);

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

    flash_test_init();



//    printf("System is initialized!\n");

//    program_flash_test();
//    eeprom_test();

//    SystemSoftwareReset();


    for(;;)
    {
    	PC2UART_receiver_run();
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
//	LPUART_DRV_SendDataPolling(INST_LPUART0, &rxData, 1);
	LPUART_DRV_SendData(INST_LPUART0, &rxData, 1);
	while( STATUS_BUSY == LPUART_DRV_GetTransmitStatus(INST_LPUART0, &bytesTransmitted) );
	uart_status = LPUART_DRV_GetTransmitStatus(INST_LPUART0, &bytesTransmitted);
	LPUART_DRV_SendDataPolling(INST_LPUART0, (uint8_t *)"\r\n", 2);	// new line
	PINS_DRV_TogglePins(PTD, 1<<6);
}

void flash_test_init(void)
{
	status_t flash_status = 0;
    // Flash initialization
	flash_status = FLASH_DRV_Init(&Flash_InitConfig0, &flashSSDConfig);
    if(flashSSDConfig.EEESize == 0u)
    {
        /* Configure FlexRAM as EEPROM and FlexNVM as EEPROM backup region,
           DEFlashPartition will be failed if the IFR region isn't blank.
           Refer to the device document for valid EEPROM Data Size Code
           and FlexNVM Partition Code. For example on S32K144:
           - EEEDataSizeCode = 0x02u: EEPROM size = 4 Kbytes
           - DEPartitionCode = 0x08u: EEPROM backup size = 64 Kbytes
         */
    	flash_status = FLASH_DRV_DEFlashPartition(&flashSSDConfig, 0x02u, 0x08u, 0x00u, false, true);
        /* Reinitialize the flash to update the new EEPROM configuration */
        flash_status = FLASH_DRV_Init(&Flash_InitConfig0, &flashSSDConfig);
        /* Make FlexRAM available for EEPROM */
        flash_status = FLASH_DRV_SetFlexRamFunction(&flashSSDConfig, EEE_ENABLE, 0x00u, NULL);
    }
}

void program_flash_test(void)
{
	uint32_t flash_program_address_1 = 0x0005F090;
	uint32_t flash_program_address_2 = flash_program_address_1 + 32;
	uint32_t flash_program_sector_start_address = 0;
	uint32_t flash_protection_status = 0;
	status_t flash_status = 0;
//	uint32_t checksum = 0;
	uint32_t failAddress = 0;
	uint8_t i = 0;
	uint8_t readDataArray1[32] = {0};
	uint8_t readDataArray2[32] = {0};
	FLASH_DRV_GetPFlashProtection(&flash_protection_status);
	flash_program_sector_start_address = flash_program_address_1 - (flash_program_address_1 % FEATURE_FLS_PF_BLOCK_SECTOR_SIZE);
	flash_status = FLASH_DRV_EraseSector(&flashSSDConfig, flash_program_sector_start_address, FEATURE_FLS_PF_BLOCK_SECTOR_SIZE);
//	flash_status = FLASH_DRV_Program(&flashSSDConfig, flash_program_address, sizeof(test_text), test_text);
//	flash_status = FLASH_DRV_ProgramCheck(&flashSSDConfig, flash_program_address, sizeof(test_text), test_text, &failaddress, 0x01);
	flash_status = FLASH_DRV_Program(&flashSSDConfig, flash_program_address_1, 32, test_text);
	flash_status = FLASH_DRV_ProgramCheck(&flashSSDConfig, flash_program_address_1, 32, test_text, &failAddress, 0x01);
//	flash_status = FLASH_DRV_CheckSum(&flashSSDConfig, flash_program_address, sizeof(test_text), &checksum);
	for( i = 0; i < 32; i++ )
	{
		readDataArray1[i] = *((uint8_t *)flash_program_address_1 + i);
	}
	/* Try to write to the same memory area will result in write error */
//	flash_status = FLASH_DRV_Program(&flashSSDConfig, flash_program_address_1 + 16, 16, delete_text);
//	flash_status = FLASH_DRV_ProgramCheck(&flashSSDConfig, flash_program_address_1 + 16, 16, delete_text, &failAddress, 0x01);

	flash_status = FLASH_DRV_Program(&flashSSDConfig, flash_program_address_2, 32, test_text);
	flash_status = FLASH_DRV_ProgramCheck(&flashSSDConfig, flash_program_address_2, 32, test_text, &failAddress, 0x01);
	for( i = 0; i < 32; i++ )
	{
		readDataArray2[i] = *((uint8_t *)flash_program_address_2 + i);
	}
	flash_status = FLASH_DRV_EraseSector(&flashSSDConfig, flash_program_sector_start_address, FEATURE_FLS_PF_BLOCK_SECTOR_SIZE);
}

void eeprom_test(void)
{
	uint32_t eeprom_address_1 = 0;
	uint32_t eeprom_address_2 = 0;
	uint32_t size = 0;
	uint8_t write_data = 0;
	uint8_t read_data = 0;
	status_t flash_status = 0;
    /* Try to write data to EEPROM if FlexRAM is configured as EEPROM */
    if (flashSSDConfig.EEESize != 0u)
    {
        eeprom_address_1 = flashSSDConfig.EERAMBase;
        size = sizeof(uint8_t);
        write_data = 0x52u;
        flash_status = FLASH_DRV_EEEWrite(&flashSSDConfig, eeprom_address_1, size, &write_data);

        /* Read the written data */
        read_data = *((uint8_t *)eeprom_address_1);
        /* Try to update one byte in an EEPROM address which isn't aligned */
        eeprom_address_2 = eeprom_address_1 + 1u;
        size = sizeof(uint8_t);
        write_data = 0x75u;
        flash_status = FLASH_DRV_EEEWrite(&flashSSDConfig, eeprom_address_2, size, &write_data);
        /* Then read */
        read_data = *((uint8_t *)eeprom_address_2);

        write_data = 0x02u;
        flash_status = FLASH_DRV_EEEWrite(&flashSSDConfig, eeprom_address_1, size, &write_data);
        flash_status = FLASH_DRV_EEEWrite(&flashSSDConfig, eeprom_address_2, size, &write_data);
        read_data = *((uint8_t *)eeprom_address_1);
        read_data = *((uint8_t *)eeprom_address_2);
    }
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
