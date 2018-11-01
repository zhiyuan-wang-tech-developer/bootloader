/*
 * bootloader.h
 *
 *  Created on: Oct 23, 2018
 *      Author: dynatron2018
 */

#ifndef BOOTLOADER_H_
#define BOOTLOADER_H_

#include "stdbool.h"
#include "stdint.h"

typedef struct
{
	uint8_t 	isNewFirmwareUpdated;
	uint32_t 	newFirmwareSize;
	uint32_t 	newFirmwareChecksum;
} NEW_FIRMWARE_STATUS_t;

// Public global variables
extern NEW_FIRMWARE_STATUS_t new_firmware_status;

// Public function prototypes
bool flash_init(void);
void JumpToOldFirmware(void);
void auto_debug_reset(void);

// For flash test purpose
void program_flash_test(void);
void emulated_eeprom_test(void);
void firmware_update_test(void);

#endif /* BOOTLOADER_H_ */
