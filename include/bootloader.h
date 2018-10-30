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

bool flash_init(void);

// For flash test purpose
void program_flash_test(void);
void emulated_eeprom_test(void);

#endif /* BOOTLOADER_H_ */
