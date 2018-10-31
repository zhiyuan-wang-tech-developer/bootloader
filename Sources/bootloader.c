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

// The size of one sector = 4096 bytes = 4KB
#define FLASH_SECTOR_SIZE			(4096u) 					// FEATURE_FLS_PF_BLOCK_SECTOR_SIZE
// The total number of sectors = 128
#define FLASH_SECTOR_NUM			(128u)
#define FLASH_WRITE_DATA_SIZE		(64u)

/*
 * The bootloader is stored at the sector 0...3 in the address range (0x0000_0000 ~ 0x0000_3FFF)
 * The bootloader maximum code size = 4 * 4096 = 16384 = 16KB
 *
 * The old firmware is stored at the sector 4...65 in the address range (0x0000_4000 ~ 0x0004_1FFF)
 * The old firmware maximum code size = 62 * 4096 = 253952 = 248KB
 *
 * The new firmware is stored at the sector 66...127 in the address range (0x0004_2000 ~ 0x0007_FFFF)
 * The new firmware maximum code size = 62 * 4096 = 253952 = 248KB
 */
#define OLD_FIRMWARE_START_ADDRESS		(0x00004000u)
#define OLD_FIRMWARE_START_SECTOR		(4u)
#define OLD_FIRMWARE_END_SECTOR			(65u)
#define OLD_FIRMWARE_MAX_SIZE			(253952u)
#define NEW_FIRMWARE_START_ADDRESS		(0x00042000u)
#define NEW_FIRMWARE_START_SECTOR		(66u)
#define NEW_FIRMWARE_END_SECTOR			(127u)
#define NEW_FIRMWARE_MAX_SIZE			(253952u)

// The new firmware status is stored in EEEPROM
#define NEW_FIRMWARE_STATUS_START_ADDRESS			(0x14000000u)
#define NEW_FIRMWARE_STATUS_UPDATE_FLAG_ADDRESS 	(NEW_FIRMWARE_STATUS_START_ADDRESS)
#define NEW_FIRMWARE_STATUS_SIZE_ADDRESS			(NEW_FIRMWARE_STATUS_START_ADDRESS + 1u)
#define NEW_FIRMWARE_STATUS_CHECKSUM_ADDRESS		(NEW_FIRMWARE_STATUS_START_ADDRESS + 5u)

// Text for flash test
uint8_t test_text[64] = "allround technology autoliv test....s32k144 firmware update test";

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
bool flash_check_write_64bytes(void);

void flash_auto_write_64bytes_reset(void);
bool flash_auto_write_64bytes(void);
bool flash_overwrite_old_firmware(void);

bool flash_erase_sector(uint8_t sectorIndex);
bool flash_erase_old_firmware(void);
bool flash_erase_new_firmware(void);

uint32_t calculateNewFirmwareSize(void);
bool calculateNewFirmwareChecksum(uint32_t * pChecksum);
bool isOldFirmwareCorrect(void);

bool eeprom_read_new_firmware_status(void);
bool eeprom_write_new_firmware_status(void);

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
	/* Try to write to the same memory area will result in write error */

	flash_status = FLASH_DRV_Program(&flashSSDConfig, flash_program_address_2, 32, test_text);
	flash_status = FLASH_DRV_ProgramCheck(&flashSSDConfig, flash_program_address_2, 32, test_text, &failAddress, 0x01);
	for( i = 0; i < 32; i++ )
	{
		readDataArray2[i] = *((uint8_t *)flash_program_address_2 + i);
	}
	flash_status = FLASH_DRV_EraseSector(&flashSSDConfig, flash_program_sector_start_address, FEATURE_FLS_PF_BLOCK_SECTOR_SIZE);
}

void emulated_eeprom_test(void)
{
	uint32_t eeprom_address_1 = 0;
	uint32_t eeprom_address_2 = 0;
	uint32_t size = 0;
	uint8_t write_data = 0;
	uint8_t read_data = 0;
	status_t flash_status = 0;
    /* Try to write data to EEPROM if FlexRAM is configured as Emulated EEPROM */
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

// Write Program Flash
bool flash_writeBytes(uint32_t writeStartAddress, uint32_t writeByteNum, uint8_t * pBufferToWrite)
{
	status_t flash_status = STATUS_SUCCESS;
	uint32_t failAddress = 0;	// Hold the failed write address
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

	if(pBufferToWrite == NULL)
	{
		return false;
	}

	// Write data to the flash
	flash_status = FLASH_DRV_Program(&flashSSDConfig, writeStartAddress, writeByteNum, pBufferToWrite);
	if( flash_status != STATUS_SUCCESS )
	{
		return false;
	}
	// Check data written to the flash
	flash_status = FLASH_DRV_ProgramCheck(&flashSSDConfig, writeStartAddress, writeByteNum, pBufferToWrite, &failAddress, 0x01u);
	if( flash_status != STATUS_SUCCESS )
	{
		return false;
	}
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
 * After  have been written to the flash memory from the flash write buffer,
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
	memcpy(flash_ReadBuffer, (void *)flash_LastWrite64BytesStartAddress, 64);
	retValue = memcmp(flash_ReadBuffer, flash_WriteBuffer, 64);
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
	flash_status = FLASH_DRV_Program(&flashSSDConfig, OLD_FIRMWARE_START_ADDRESS, firmwareSize, (uint8_t *)NEW_FIRMWARE_START_ADDRESS);
	if( flash_status != STATUS_SUCCESS )
	{
		return false;
	}
	flash_status = FLASH_DRV_ProgramCheck(&flashSSDConfig, OLD_FIRMWARE_START_ADDRESS, firmwareSize, (uint8_t *)NEW_FIRMWARE_START_ADDRESS, &failAddress, 0x01);
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
	flash_status = FLASH_DRV_EraseSector(&flashSSDConfig, flash_ErasedSectorStartAddress, FLASH_SECTOR_SIZE);
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
    eeprom_status = FLASH_DRV_EEEWrite(&flashSSDConfig, NEW_FIRMWARE_STATUS_UPDATE_FLAG_ADDRESS, sizeof(uint8_t), (uint8_t *)&new_firmware_status.isNewFirmwareUpdated);
	if( eeprom_status != STATUS_SUCCESS )
	{
		return false;
	}
	// Write new firmware size
    eeprom_status = FLASH_DRV_EEEWrite(&flashSSDConfig, NEW_FIRMWARE_STATUS_SIZE_ADDRESS, sizeof(uint32_t), (uint8_t *)&new_firmware_status.newFirmwareSize);
	if( eeprom_status != STATUS_SUCCESS )
	{
		return false;
	}
	// Write new firmware checksum
    eeprom_status = FLASH_DRV_EEEWrite(&flashSSDConfig, NEW_FIRMWARE_STATUS_CHECKSUM_ADDRESS, sizeof(uint32_t), (uint8_t *)&new_firmware_status.newFirmwareChecksum);
	if( eeprom_status != STATUS_SUCCESS )
	{
		return false;
	}
	return true;
}

void firmware_update_test(void)
{
	bool retValue = false;
	retValue = eeprom_read_new_firmware_status();
	if(!retValue)
	{
		return;
	}
	if(new_firmware_status.isNewFirmwareUpdated == 1u)
	{
		// Copy new firmware to overwrite old firmware.
		retValue = flash_overwrite_old_firmware();
		if(!retValue)
		{
			return;
		}
		// After the new firmware is copied to overwrite the old firmware, clear the update flag.
		new_firmware_status.isNewFirmwareUpdated = 0u;
	}
	// Store the new firmware status into eeprom
	retValue = eeprom_write_new_firmware_status();
	if(!retValue)
	{
		return;
	}

	// Fill up raw data in the rx data packet with dummy data for test purpose.
	memcpy(rx_data_packet.item.raw_data, test_text, sizeof(test_text));
	// Start new firmware writing
	uint8_t i = 0;
	for(i = 0; i < 5; i++)
	{
		retValue = flash_auto_write_64bytes();
		if(!retValue)
		{
			// fail in auto write
			return;
		}
	}
	// End new firmware writing
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
	// Store the new firmware status into eeprom
	retValue = eeprom_write_new_firmware_status();
	if(!retValue)
	{
		return;
	}
}
