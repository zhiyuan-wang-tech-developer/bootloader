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
#define OLD_FIRMWARE_MAX_SIZE			(253952u)
#define NEW_FIRMWARE_START_ADDRESS		(0x00042000u)
#define NEW_FIRMWARE_MAX_SIZE			(253952u)


// Text for flash test
uint8_t test_text[] = "allround technology autoliv test\r\n";


// For 64-bytes data writing and reading
static uint8_t flash_WriteBuffer[64] = {0};
static uint8_t flash_ReadBuffer[64] = {0};

static uint32_t flash_LastErasedSectorStartAddress = 0u;
static uint32_t flash_LastWrite64BytesStartAddress = 0u;
static uint32_t flash_Write64BytesCount = 0u;
static uint32_t flash_CurrentWrite64BytesStartAddress = 0u;
static uint32_t flash_CurrentSector = 0u; // Sector 0...127

// flash module static
flash_ssd_config_t flashSSDConfig;

/*
 * Private Function Prototype
 */
bool flash_write(uint32_t writeStartAddress, uint32_t writeByteNum, uint8_t * pBufferToWrite);
uint8_t flash_readByte(uint32_t readAddress);
void flash_load_write_buffer(void);

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
bool flash_write(uint32_t writeStartAddress, uint32_t writeByteNum, uint8_t * pBufferToWrite)
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

bool flash_auto_write_64bytes(void)
{
	bool retStatus = false;
	status_t flash_status = STATUS_SUCCESS;
	uint32_t flash_CurrentErasedSectorStartAddress = 0u;

	if(flash_Write64BytesCount == 0)
	{
		// Now start the writing of the first 64 bytes.
		flash_CurrentWrite64BytesStartAddress = NEW_FIRMWARE_START_ADDRESS;
		flash_LastErasedSectorStartAddress = 0u;
	}
	else
	{
		// Some 64-bytes data blocks have been written to the flash memory
		flash_CurrentWrite64BytesStartAddress = flash_LastWrite64BytesStartAddress + FLASH_WRITE_DATA_SIZE;
	}
	// Decide which sector the current start address is in.
	flash_CurrentSector = flash_CurrentWrite64BytesStartAddress / FLASH_SECTOR_SIZE;
	// Check if it is necessary to erase the sector
	if(flash_LastErasedSectorStartAddress == 0u)
	{

		flash_CurrentErasedSectorStartAddress = flash_CurrentSector * FLASH_SECTOR_SIZE;
		flash_status = FLASH_DRV_EraseSector(&flashSSDConfig, flash_CurrentErasedSectorStartAddress, FLASH_SECTOR_SIZE);
		if( flash_status != STATUS_SUCCESS )
		{
			return false;
		}
		flash_LastErasedSectorStartAddress = flash_CurrentErasedSectorStartAddress;
	}
	else
	{
		// Check if the current write start address is in the next sector.
		if(flash_CurrentWrite64BytesStartAddress >= (flash_LastErasedSectorStartAddress + FLASH_SECTOR_SIZE))
		{
			// Now the current write start address is in the next sector
			flash_CurrentErasedSectorStartAddress = flash_CurrentSector * FLASH_SECTOR_SIZE;
			flash_status = FLASH_DRV_EraseSector(&flashSSDConfig, flash_CurrentErasedSectorStartAddress, FLASH_SECTOR_SIZE);
			if( flash_status != STATUS_SUCCESS )
			{
				return false;
			}
			flash_LastErasedSectorStartAddress = flash_CurrentErasedSectorStartAddress;
		}
	}
	// Load the flash write buffer from the rx data packet prior to flash writing
	flash_load_write_buffer();
	retStatus = flash_write(flash_CurrentWrite64BytesStartAddress, FLASH_WRITE_DATA_SIZE, flash_WriteBuffer);
	if(retStatus == false)
	{
		// Flash writing failure
		return false;
	}
	flash_LastWrite64BytesStartAddress = flash_CurrentWrite64BytesStartAddress;
	// Check if the flash write is successful
	retStatus = flash_check_write_64bytes();
	if(retStatus == false)
	{
		// Mismatch of flash writing data
		return false;
	}
	flash_Write64BytesCount++;
	// Flash writing success
	return true;
}
