/*
 * bootloader.c
 *
 *  Created on: Oct 23, 2018
 *      Author: dynatron2018
 */

#include "bootloader.h"
#include "pc_communication.h"
#include "Cpu.h"
#include "string.h"
#include "stdio.h"


/* Little-endianness to Big-endianness macro */
/*
 * After you read a word (32-bits) from flash,
 * you must convert it from little-endianness to big-endianness.
 *
 */
#define LE2BE_32(c)	(((c & 0xFF) << 24) | ((c & 0xFF00) << 8) | ((c & 0xFF0000) >> 8) | ((c & 0xFF000000) >> 24))

#define LED_OFF		PINS_DRV_ClearPins(PTE, 1<<8)
#define LED_ON		PINS_DRV_SetPins(PTE, 1<<8)

// The size of one sector = 4096 bytes = 4KB
#define FLASH_SECTOR_SIZE			(4096u) 					// FEATURE_FLS_PF_BLOCK_SECTOR_SIZE
// The total number of sectors = 128
#define FLASH_SECTOR_NUM			(128u)
#define FLASH_WRITE_DATA_SIZE		(64u)

/*
 * The bootloader is stored at the sector 0...11 in the address range (0x0000_0000 ~ 0x0000_BFFF)
 * The bootloader maximum code size = 12 * 4096 = 49152 = 48KB
 *
 * The old firmware is stored at the sector 12...69 in the address range (0x0000_C000 ~ 0x0004_5FFF)
 * The old firmware maximum code size = 58 * 4096 = 237568 = 232KB
 *
 * The new firmware is stored at the sector 70...127 in the address range (0x0004_6000 ~ 0x0007_FFFF)
 * The new firmware maximum code size = 58 * 4096 = 237568 = 232KB
 */
#define OLD_FIRMWARE_START_ADDRESS		(0x0000C000u)
#define OLD_FIRMWARE_START_SECTOR		(12u)
#define OLD_FIRMWARE_END_SECTOR			(69u)
#define OLD_FIRMWARE_MAX_SIZE			(237568u)

#define NEW_FIRMWARE_START_ADDRESS		(0x00046000u)
#define NEW_FIRMWARE_START_SECTOR		(70u)
#define NEW_FIRMWARE_END_SECTOR			(127u)
#define NEW_FIRMWARE_MAX_SIZE			(237568u)

// The new firmware status is stored in EEEPROM
#define NEW_FIRMWARE_STATUS_START_ADDRESS			(0x14000000u)
#define NEW_FIRMWARE_STATUS_UPDATE_FLAG_ADDRESS 	(NEW_FIRMWARE_STATUS_START_ADDRESS)
#define NEW_FIRMWARE_STATUS_SIZE_ADDRESS			(NEW_FIRMWARE_STATUS_START_ADDRESS + 4u)
#define NEW_FIRMWARE_STATUS_CHECKSUM_ADDRESS		(NEW_FIRMWARE_STATUS_START_ADDRESS + 8u)

// Text for flash test
uint8_t test_text[64] = "..allround technology autoliv test..s32k144 firmware update test";

NEW_FIRMWARE_STATUS_t new_firmware_status =
		{
				.isNewFirmwareUpdated = 0u,
				.newFirmwareSize = 0u,
				.newFirmwareChecksum = 0u
		};

// For 64-bytes data writing and reading
static uint8_t flash_WriteBuffer[64] = {0};
static uint8_t flash_ReadBuffer[64] = {0};

static uint32_t flash_LastWrite64BytesStartAddress = 0u;
static uint32_t flash_CurrentWrite64BytesStartAddress = 0u;
static uint32_t flash_CurrentSectorIndex = 0u; 				// Sector 0...127
static uint32_t flash_Write64BytesCount = 0u;

// flash module static
flash_ssd_config_t flashSSDConfig;

/*
 * Private Function Prototype
 */
bool flash_writeBytes(uint32_t writeStartAddress, uint32_t writeByteNum, uint8_t * pBufferToWrite);
uint8_t flash_readByte(uint32_t readAddress);
void flash_load_write_buffer(void);
void flash_write_buffer_little_endian_to_big_endian(void);
bool flash_check_write_64bytes(void);

//void flash_auto_write_64bytes_reset(void);
//bool flash_auto_write_64bytes(void);
bool flash_overwrite_old_firmware(void);

bool flash_erase_sector(uint8_t sectorIndex);
bool flash_erase_old_firmware(void);
bool flash_erase_new_firmware(void);

//uint32_t calculateNewFirmwareSize(void);
//bool calculateNewFirmwareChecksum(uint32_t * pChecksum);
bool isOldFirmwareCorrect(void);

//bool eeprom_read_new_firmware_status(void);
//bool eeprom_write_new_firmware_status(void);

void JumpToExecute(uint32_t stack_pointer, uint32_t program_counter);

void printOldFirmware(void);

// Initialize both the program flash and the data flash
bool flash_init(void)
{
	status_t flash_status = STATUS_SUCCESS;
    // The flash module initialization before invoking any other flash functions
	flash_status = FLASH_DRV_Init(&Flash_InitConfig0, &flashSSDConfig);
	if(flash_status != STATUS_SUCCESS)
	{
		return false;
	}
	// If the Emulated EEPROM size is zero, initialize the FlexNVM as EEPROM
    if(flashSSDConfig.EEESize == 0u)
    {
        /* Configure FlexRAM as Emulated EEPROM and FlexNVM as EEPROM backup region,
           DEFlashPartition will be failed if the IFR region isn't blank.
           Refer to the device document for valid EEPROM Data Size Code
           and FlexNVM Partition Code. For example on S32K144:
           - EEEDataSizeCode = 0x02u: EEPROM size = 4 Kbytes
           - DEPartitionCode = 0x08u: EEPROM backup size = 64 Kbytes
         */
    	flash_status = FLASH_DRV_DEFlashPartition(&flashSSDConfig, 0x02u, 0x08u, 0x00u, false, true);
    	if(flash_status != STATUS_SUCCESS)
    	{
    		return false;
    	}
        /* Reinitialize the flash to update the new EEPROM configuration */
        flash_status = FLASH_DRV_Init(&Flash_InitConfig0, &flashSSDConfig);
    	if(flash_status != STATUS_SUCCESS)
    	{
    		return false;
    	}
        /* Make FlexRAM available for Emulated EEPROM */
        flash_status = FLASH_DRV_SetFlexRamFunction(&flashSSDConfig, EEE_ENABLE, 0x00u, NULL);
    	if(flash_status != STATUS_SUCCESS)
    	{
    		return false;
    	}
    }
    return true;
}

/*
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

	flash_status = FLASH_DRV_Program(&flashSSDConfig, flash_program_address_1, 32, test_text);
	flash_status = FLASH_DRV_ProgramCheck(&flashSSDConfig, flash_program_address_1, 32, test_text, &failAddress, 0x01);

	for( i = 0; i < 32; i++ )
	{
		readDataArray1[i] = *((uint8_t *)flash_program_address_1 + i);
	}
	 Try to write to the same memory area will result in write error

	flash_status = FLASH_DRV_Program(&flashSSDConfig, flash_program_address_2, 32, test_text);
	flash_status = FLASH_DRV_ProgramCheck(&flashSSDConfig, flash_program_address_2, 32, test_text, &failAddress, 0x01);
	for( i = 0; i < 32; i++ )
	{
		readDataArray2[i] = *((uint8_t *)flash_program_address_2 + i);
	}
	flash_status = FLASH_DRV_EraseSector(&flashSSDConfig, flash_program_sector_start_address, FEATURE_FLS_PF_BLOCK_SECTOR_SIZE);
}
*/

/*
void emulated_eeprom_test(void)
{
	uint32_t eeprom_address_1 = 0;
	uint32_t eeprom_address_2 = 0;
	uint32_t size = 0;
	uint8_t write_data = 0;
	uint8_t read_data = 0;
	status_t flash_status = 0;
     Try to write data to EEPROM if FlexRAM is configured as Emulated EEPROM
    if (flashSSDConfig.EEESize != 0u)
    {
        eeprom_address_1 = flashSSDConfig.EERAMBase;
        size = sizeof(uint8_t);
        write_data = 0x52u;
        flash_status = FLASH_DRV_EEEWrite(&flashSSDConfig, eeprom_address_1, size, &write_data);

         Read the written data
        read_data = *((uint8_t *)eeprom_address_1);
         Try to update one byte in an EEPROM address which isn't aligned
        eeprom_address_2 = eeprom_address_1 + 1u;
        size = sizeof(uint8_t);
        write_data = 0x75u;
        flash_status = FLASH_DRV_EEEWrite(&flashSSDConfig, eeprom_address_2, size, &write_data);
         Then read
        read_data = *((uint8_t *)eeprom_address_2);

        write_data = 0x02u;
        flash_status = FLASH_DRV_EEEWrite(&flashSSDConfig, eeprom_address_1, size, &write_data);
        flash_status = FLASH_DRV_EEEWrite(&flashSSDConfig, eeprom_address_2, size, &write_data);
        read_data = *((uint8_t *)eeprom_address_1);
        read_data = *((uint8_t *)eeprom_address_2);
    }
}
*/

// Write Program Flash
bool flash_writeBytes(uint32_t writeStartAddress, uint32_t writeByteNum, uint8_t * pBufferToWrite)
{
	status_t flash_status = STATUS_SUCCESS;
//	uint32_t failAddress = 0;	// Hold the failed write address
	// Safety Check
	if( (writeStartAddress % 8u) != 0u )
	{
		/*
		 * The write start address must be 8-bytes aligned,
		 * otherwise FLASH_DRV_Program() return ERROR.
		 */
		return false;
	}

	if( writeStartAddress > 0x0007FFFFu )
	{
		// The write address is not in the range of program flash (512KB)
		return false;
	}

	if( (writeByteNum % FEATURE_FLS_PF_BLOCK_WRITE_UNIT_SIZE) != 0u )
	{
		/*
		 * The write byte number (data size) must be 8-bytes aligned,
		 * otherwise FLASH_DRV_Program() return UNSUPPORTED.
		 */
		return false;
	}

	if( pBufferToWrite == NULL )
	{
		return false;
	}

	// Write data to the flash
	INT_SYS_DisableIRQGlobal();
	flash_status = FLASH_DRV_Program(&flashSSDConfig, writeStartAddress, writeByteNum, pBufferToWrite);
    INT_SYS_EnableIRQGlobal();
	if( flash_status != STATUS_SUCCESS )
	{
		return false;
	}

	// Check data written to the flash
//	INT_SYS_DisableIRQGlobal();
//	flash_status = FLASH_DRV_ProgramCheck(&flashSSDConfig, writeStartAddress, writeByteNum, pBufferToWrite, &failAddress, 0x01u);
//  INT_SYS_EnableIRQGlobal();
//	if( flash_status != STATUS_SUCCESS )
//	{
//		return false;
//	}
	return true;
}

// Read a byte from the flash memory address
uint8_t flash_readByte(uint32_t readAddress)
{
	uint8_t readByte = 0;
	readByte = *((uint8_t *)readAddress);
	return readByte;
}

/*
 * Load 64-bytes raw data from the UART rx data packet to the flash write buffer.
 *
 * loading write buffer should be done prior to flash_auto_write_64bytes().
 */
void flash_load_write_buffer(void)
{
	memcpy(flash_WriteBuffer, rx_data_packet.item.raw_data, 64u);
}

/*
 * The write buffer has 64 bytes or 16 words (32-bits)
 * Every word in the write buffer must be converted from little-endianness to big-endianness.
 * After the little-endianness to big-endianness conversion,
 * then you can write the contents in the write buffer to flash.
 *
 */
void flash_write_buffer_little_endian_to_big_endian(void)
{
	uint32_t * write_buffer_word = (uint32_t *)flash_WriteBuffer;
	uint8_t i = 0;
	for(i = 0; i < 16; i++)
	{
		write_buffer_word[i] = LE2BE_32(write_buffer_word[i]);
	}
}

/*
 * After 64-bytes data have been written to the flash memory from the flash write buffer,
 * read the 64 bytes into the flash read buffer,
 * then compare the flash read buffer and the flash write buffer.
 *
 * @return:
 * 		true: 	success in writing 64 bytes
 * 		false:  error in writing 64 bytes
 */
bool flash_check_write_64bytes(void)
{
	int retValue = 0;
	uint8_t i = 0;
	uint8_t * checkStartAddress = (uint8_t *)flash_LastWrite64BytesStartAddress;

	INT_SYS_DisableIRQGlobal();
	//Note: The memory copy sometimes failed, so suggest not to use it anymore.
	//memcpy(flash_ReadBuffer, (uint8_t *)flash_LastWrite64BytesStartAddress, 64u);
	for( i = 0; i < 64u; i++ )
	{
		// Read 64 bytes from flash.
		flash_ReadBuffer[i] = checkStartAddress[i];
	}
	//retValue = memcmp(flash_ReadBuffer, flash_WriteBuffer, 64u);
	for( i = 0; i < 64u; i++ )
	{
		if( flash_ReadBuffer[i] != flash_WriteBuffer[i] )
		{
			retValue++;
		}
	}
	INT_SYS_EnableIRQGlobal();

	if( retValue == 0 )
	{
		// The data in both flash_ReadBuffer and flash_WriteBuffer are equal.
		return true;
	}
	else
	{
		return false;
	}
}

/*
 * If you want flash_auto_write_64bytes() function to rewrite data from the New Firmware Start Address again,
 * call this function.
 *
 */
void flash_auto_write_64bytes_reset(void)
{
	flash_Write64BytesCount = 0;
}

/*
 * It continuously fills 64-bytes data into flash memory in new firmware area
 * every time when you call it.
 */
bool flash_auto_write_64bytes(void)
{
	bool retValue = false;

	if(flash_Write64BytesCount == 0)
	{
		// Now start the writing of the first 64 bytes.
		flash_CurrentWrite64BytesStartAddress = NEW_FIRMWARE_START_ADDRESS;
		retValue = flash_erase_new_firmware();
		if(retValue == false)
		{
			// Flash erasing failure
			return false;
		}
	}
	else
	{
		// Some 64-bytes data blocks have been written to the flash memory
		flash_CurrentWrite64BytesStartAddress = flash_LastWrite64BytesStartAddress + FLASH_WRITE_DATA_SIZE;
	}
	// Decide which sector the current start address is in.
	flash_CurrentSectorIndex = flash_CurrentWrite64BytesStartAddress / FLASH_SECTOR_SIZE;
	// Load the flash write buffer from the rx data packet prior to flash writing
	flash_load_write_buffer();
//	flash_write_buffer_little_endian_to_big_endian();

	// Write the data block to the flash
	retValue = flash_writeBytes(flash_CurrentWrite64BytesStartAddress, FLASH_WRITE_DATA_SIZE, flash_WriteBuffer);
	if(retValue == false)
	{
		// Flash writing failure
		return false;
	}
	flash_LastWrite64BytesStartAddress = flash_CurrentWrite64BytesStartAddress;

	// Check if the flash write is successful
	retValue = flash_check_write_64bytes();
	if(retValue == false)
	{
		// Mismatch in flash writing data
		return false;
	}

	flash_Write64BytesCount++;
	// Flash writing success
	return true;
}

/*
 * Copy the new firmware and overwrite the old firmware
 */
bool flash_overwrite_old_firmware(void)
{
	bool retValue = false;
	uint32_t firmwareSize = 0;
	status_t flash_status = STATUS_SUCCESS;
	uint32_t failAddress = 0;
	// Erase the old firmware area
	retValue = flash_erase_old_firmware();
	if(retValue == false)
	{
		// Fail to erase old flash firmware.
		return false;
	}
	// Get new firmware size in bytes
	firmwareSize = new_firmware_status.newFirmwareSize;
	// Start to copy new firmware to old firmware area
	// Critical section where all interrupts are forbiden.
	INT_SYS_DisableIRQGlobal();
	flash_status = FLASH_DRV_Program(&flashSSDConfig, OLD_FIRMWARE_START_ADDRESS, firmwareSize, (uint8_t *)NEW_FIRMWARE_START_ADDRESS);
	INT_SYS_EnableIRQGlobal();
	if( flash_status != STATUS_SUCCESS )
	{
		return false;
	}
	// Critical section where all interrupts are forbiden.
	INT_SYS_DisableIRQGlobal();
	flash_status = FLASH_DRV_ProgramCheck(&flashSSDConfig, OLD_FIRMWARE_START_ADDRESS, firmwareSize, (uint8_t *)NEW_FIRMWARE_START_ADDRESS, &failAddress, 0x01);
	INT_SYS_EnableIRQGlobal();
	if( flash_status != STATUS_SUCCESS )
	{
		return false;
	}
	if(!isOldFirmwareCorrect())
	{
		return false;
	}
	return true;
}

/*
 * Erase the specified flash sector
 * @param:
 * 		sectorIndex: 0 ~ 127 corresponding to each 4kB sector
 */
bool flash_erase_sector(uint8_t sectorIndex)
{
	uint32_t flash_ErasedSectorStartAddress = 0;
	status_t flash_status = STATUS_SUCCESS;
	if(sectorIndex > 127u)
	{
		return false;
	}
	flash_ErasedSectorStartAddress = sectorIndex * FLASH_SECTOR_SIZE;
	// Critical section where all interrupts are forbiden.
	INT_SYS_DisableIRQGlobal();
	flash_status = FLASH_DRV_EraseSector(&flashSSDConfig, flash_ErasedSectorStartAddress, FLASH_SECTOR_SIZE);
	INT_SYS_EnableIRQGlobal();
	if( flash_status != STATUS_SUCCESS )
	{
		return false;
	}
	return true;
}

bool flash_erase_old_firmware(void)
{
	uint8_t sectorIndex = 0;
	bool retValue = false;
	for(sectorIndex = OLD_FIRMWARE_START_SECTOR; sectorIndex <= OLD_FIRMWARE_END_SECTOR; sectorIndex++)
	{
		retValue = flash_erase_sector(sectorIndex);
		if(retValue == false)
		{
			return false;
		}
	}
	return true;
}

bool flash_erase_new_firmware(void)
{
	uint8_t sectorIndex = 0;
	bool retValue = false;
	for(sectorIndex = NEW_FIRMWARE_START_SECTOR; sectorIndex <= NEW_FIRMWARE_END_SECTOR; sectorIndex++)
	{
		retValue = flash_erase_sector(sectorIndex);
		if(retValue == false)
		{
			return false;
		}
	}
	return true;
}

/*
 * Get the size of the firmware that you have written to flash
 */
uint32_t calculateNewFirmwareSize(void)
{
	uint32_t size = 0;
	size = flash_Write64BytesCount * FLASH_WRITE_DATA_SIZE;
	return size;
}

bool calculateNewFirmwareChecksum(uint32_t * pChecksum)
{
	if(pChecksum == NULL)
	{
		return false;
	}
	uint32_t newFirmwareSize = 0;
	status_t flash_status = STATUS_SUCCESS;
	newFirmwareSize = flash_Write64BytesCount * FLASH_WRITE_DATA_SIZE;
	flash_status = FLASH_DRV_CheckSum(&flashSSDConfig, NEW_FIRMWARE_START_ADDRESS, newFirmwareSize, pChecksum);
	if( flash_status != STATUS_SUCCESS )
	{
		return false;
	}
	return true;
}

/*
 * Compare the old firmware checksum with the new firmware checksum
 */
bool isOldFirmwareCorrect(void)
{
	uint32_t oldFirmwareSize = 0;
	uint32_t oldFirmwareChecksum = 0;
	status_t flash_status = STATUS_SUCCESS;
	oldFirmwareSize = new_firmware_status.newFirmwareSize;
	flash_status = FLASH_DRV_CheckSum(&flashSSDConfig, OLD_FIRMWARE_START_ADDRESS, oldFirmwareSize, &oldFirmwareChecksum);
	if( flash_status != STATUS_SUCCESS )
	{
		return false;
	}
	if( oldFirmwareChecksum != new_firmware_status.newFirmwareChecksum )
	{
		return false;
	}
	return true;
}

/*
 * Read new firmware status from EEPROM
 */
bool eeprom_read_new_firmware_status(void)
{
    /* Try to read data from EEPROM if FlexRAM is configured as Emulated EEPROM */
    if (flashSSDConfig.EEESize == 0u)
    {
    	// No Emulated EEPROM is assigned
    	return false;
    }
    new_firmware_status.isNewFirmwareUpdated = 	*((uint8_t  *)NEW_FIRMWARE_STATUS_UPDATE_FLAG_ADDRESS);
    new_firmware_status.newFirmwareSize = 		*((uint32_t *)NEW_FIRMWARE_STATUS_SIZE_ADDRESS);
    new_firmware_status.newFirmwareChecksum = 	*((uint32_t *)NEW_FIRMWARE_STATUS_CHECKSUM_ADDRESS);
    return true;
}

/*
 * Write new firmware status to EEPROM
 */
bool eeprom_write_new_firmware_status(void)
{
	status_t eeprom_status = STATUS_SUCCESS;

	// Write new firmware update flag
	// Critical section where all interrupts are forbiden.
	INT_SYS_DisableIRQGlobal();
    eeprom_status = FLASH_DRV_EEEWrite(&flashSSDConfig, NEW_FIRMWARE_STATUS_UPDATE_FLAG_ADDRESS, sizeof(uint8_t), (uint8_t *)&new_firmware_status.isNewFirmwareUpdated);
    INT_SYS_EnableIRQGlobal();
    if( eeprom_status != STATUS_SUCCESS )
	{
		return false;
	}

	// Write new firmware size
	// Critical section where all interrupts are forbiden.
	INT_SYS_DisableIRQGlobal();
    eeprom_status = FLASH_DRV_EEEWrite(&flashSSDConfig, NEW_FIRMWARE_STATUS_SIZE_ADDRESS, sizeof(uint32_t), (uint8_t *)&new_firmware_status.newFirmwareSize);
    INT_SYS_EnableIRQGlobal();
    if( eeprom_status != STATUS_SUCCESS )
	{
		return false;
	}

	// Write new firmware checksum
	// Critical section where all interrupts are forbiden.
	INT_SYS_DisableIRQGlobal();
    eeprom_status = FLASH_DRV_EEEWrite(&flashSSDConfig, NEW_FIRMWARE_STATUS_CHECKSUM_ADDRESS, sizeof(uint32_t), (uint8_t *)&new_firmware_status.newFirmwareChecksum);
    INT_SYS_EnableIRQGlobal();
    if( eeprom_status != STATUS_SUCCESS )
	{
		return false;
	}
	return true;
}

void firmware_update_test(void)
{
	char InputChar = 'n';
	bool retValue = false;

	retValue = eeprom_read_new_firmware_status();
	if(!retValue)
	{
		return;
	}

	if(new_firmware_status.isNewFirmwareUpdated == 1u)
	{
		printf("Start new firmware update...\r\n");
		// Copy new firmware to overwrite old firmware.
		retValue = flash_overwrite_old_firmware();
		if(!retValue)
		{
			return;
		}
		printf("End new firmware update...\r\n");

		// After the new firmware is copied to overwrite the old firmware, clear the update flag.
		new_firmware_status.isNewFirmwareUpdated = 0u;

		// Store the new firmware status into EEPROM
		retValue = eeprom_write_new_firmware_status();
		if(!retValue)
		{
			return;
		}

		printOldFirmware();
	}
	else
	{
		printf("No firmware updated.\r\n");
	}

	printf("Do you want to update new firmware? (y/n)\r\n");
	fflush(stdin);
	InputChar = getchar();
	if(InputChar == 'y')
	{
		// Fill up raw data in the rx data packet with dummy data for test purpose.
		memcpy(rx_data_packet.item.raw_data, test_text, sizeof(test_text));
		// Start new firmware writing
		uint8_t i = 0;
		for(i = 0; i < 7; i++)
		{
			retValue = flash_auto_write_64bytes();
			if(!retValue)
			{
				printf("fail to write 64 bytes data: %lu\r\n", flash_Write64BytesCount + 1);
				// fail in auto write
				return;
			}
		}
		// End new firmware writing
		printf("Succeed in new firmware writing.\r\n");
		// New firmware is updated
		new_firmware_status.isNewFirmwareUpdated = 1u;
		// Calculate the size of new firmware
		new_firmware_status.newFirmwareSize = calculateNewFirmwareSize();
		// Calculate the checksum of new firmware
		uint32_t checksum = 0;
		if(true == calculateNewFirmwareChecksum(&checksum))
		{
			new_firmware_status.newFirmwareChecksum = checksum;
		}
		// Store the new firmware status into EEPROM for use in next restart.
		retValue = eeprom_write_new_firmware_status();
		if(!retValue)
		{
			return;
		}
	}
//	auto_debug_reset();
//	JumpToOldFirmware();
}

void firmware_update(void)
{
//    FLASH_DRV_EraseAllBlock(&flashSSDConfig);
	bool retValue = false;

	retValue = eeprom_read_new_firmware_status();
	if(!retValue)
	{
#ifdef DEBUG_FROM_RAM
//		printf("Fail to read EEPROM!\r\n");
#endif
		retValue = eeprom_read_new_firmware_status();
	}

	if(new_firmware_status.isNewFirmwareUpdated == 1u)
	{
#ifdef DEBUG_FROM_RAM
//		printf("Start new firmware updating...\r\n");
#endif
		// Copy new firmware to overwrite old firmware.
		retValue = flash_overwrite_old_firmware();
		if(!retValue)
		{
#ifdef DEBUG_FROM_RAM
//			printf("Failure in new firmware copying...!\r\n");
#endif
			// Copy new firmware again if the previous copy failed.
			retValue = flash_overwrite_old_firmware();
		}
#ifdef DEBUG_FROM_RAM
//		printf("End new firmware updating...\r\n");
#endif
		// After the new firmware is copied to overwrite the old firmware, clear the update flag.
		new_firmware_status.isNewFirmwareUpdated = 0u;

		// Store the new firmware status into EEPROM
		retValue = eeprom_write_new_firmware_status();
		if(!retValue)
		{
#ifdef DEBUG_FROM_RAM
//			printf("Failure to write EEPROM!\r\n");
#endif
			retValue = eeprom_write_new_firmware_status();
		}

#ifdef DEBUG_FROM_RAM
		printf("Auto reset\r\n");
		auto_ram_reset();
#endif

#ifdef RUN_FROM_FLASH
		auto_flash_reset();
#endif
	}
	else
	{
#ifdef DEBUG_FROM_RAM
		printf("No firmware updated\r\n");
		printf("Jump to old firmware\r\n");
#endif
		/*
		 * If there exists an old firmware, jump to the old firmware and this function will never return.
		 * If no old firmware exists, this function will return.
		 */
		JumpToOldFirmware();
	}
}

/*
 * @brief: Used to jump to the entry point of the old firmware application
 * 		   The vector table of the old firmware application is located at 0x0000_C000 (old firmware start address)
 *
 */
void JumpToOldFirmware(void)
{
	// Local variables
	uint32_t * startAddress = (uint32_t *)OLD_FIRMWARE_START_ADDRESS;
	uint32_t userStackPointer = 0u;
	uint32_t userProgramCounter = 0u;

	// Important: After you read a 32-bits word from flash, you must convert it from little-endian to big-endian...
	// The first 32-bits word in the old firmware start address is loaded into stack pointer (SP)
//	userStackPointer = LE2BE_32(startAddress[0]);
	// The second 32-bits word in the old firmware start address is loaded into program counter (PC)
//	userProgramCounter = LE2BE_32(startAddress[1]);

	userStackPointer = startAddress[0];
	userProgramCounter = startAddress[1];
	/*
	 * Check if the word in entry address is erased,
	 * and return if erased.
	 */
	if( userStackPointer == 0xFFFFFFFF )
	{
		return;
	}

	if( userStackPointer != 0x20007000 )
	{
		// The stack pointer must point to the top of stack, that is, the buttom of SRAM_U.
		return;
	}

	/*
	 * PC offset = 0x411
	 */
	if( userProgramCounter != (OLD_FIRMWARE_START_ADDRESS + 0x00000411) )
	{
		// The program counter must point to the reset handler entry address (firmware start address + offset).
		return;
	}

	/*
	 *	Note:
	 *
	 * 	The 32-bit general-purpose registers for data operations: R0 ~ R12
	 *
	 * 	The Stack Pointer (SP) register: R13
	 *		* MSP:	Main Stack Pointer. This is the reset value.
	 *		* PSP:  Process Stack Pointer.
	 *	On reset, the ARM core loads the MSP (Main Stack Pointer) with the value from start address 0x0000_4000.
	 *
	 *	The Program Counter (PC) register: R15.
	 *	PC contains the current program address.
	 *	On reset, the ARM core loads the PC with the value of the reset vector, which is at start address 0x0000_4004.
	 *
	 */

	/* Relocate vector table in vector table offset register */
	S32_SCB->VTOR = (uint32_t)startAddress;

	JumpToExecute(userStackPointer, userProgramCounter);
}

/*
 * Only for debug mode when s32k144 is running from RAM.
 *
 * SRAM start address = 0x1FFF_8000
 */
void auto_ram_reset(void)
{
	// Local variables
	uint32_t *startAddress = (uint32_t *) 0x1FFF8000u;
	uint32_t stackPointer = startAddress[0];
	uint32_t programCounter = startAddress[1];

	/* Relocate vector table in vector table offset register */
	S32_SCB->VTOR = (uint32_t)startAddress;

	JumpToExecute(stackPointer, programCounter);
}

/*
 * Only used when s32k144 is running from FLASH.
 *
 * Bootloader start address = 0x0000_0000
 */
void auto_flash_reset(void)
{
	// Local variables
	uint32_t *startAddress = (uint32_t *) 0x00000000u;
	uint32_t stackPointer = startAddress[0];
	uint32_t programCounter = startAddress[1];

	/* Relocate vector table in vector table offset register */
	S32_SCB->VTOR = (uint32_t)startAddress;

	JumpToExecute(stackPointer, programCounter);
}

void JumpToExecute(uint32_t stack_pointer, uint32_t program_counter)
{
	// Register r0 holds argument 0 stack_pointer
	// Register r1 holds argument 1 program counter
	/* Set up stack pointer (SP) */
	__asm("msr msp, r0");
//	__asm("msr psp, r0");
	/* Program counter (PC) jumps to application start address (r1) */
	__asm("mov pc, r1");
}

void printOldFirmware(void)
{
	uint8_t * startAddress = (uint8_t *) OLD_FIRMWARE_START_ADDRESS;
	uint32_t i = 0;
	printf("Current firmware to execute:");
	for(i = 0; i < new_firmware_status.newFirmwareSize; i++)
	{
		if( i%64 == 0 )
		{
			// Every 64 bytes are printed on each row.
			printf("\r\n");
		}
		printf("%c ", startAddress[i]);
	}
	printf("\r\n");
}
