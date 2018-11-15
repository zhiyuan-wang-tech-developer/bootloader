/*
 * system_config.h
 *
 *  Created on: Nov 14, 2018
 *      Author: dynatron2018
 */

#ifndef SYSTEM_CONFIG_H_
#define SYSTEM_CONFIG_H_

// Interrupt Priority Level Settings
#define 	INTERRUPT_PRIORITY_LEVEL_UART			(5u)			// Low Power UART Module 0 RX & TX IRQ for firmware download
#define 	INTERRUPT_PRIORITY_LEVEL_TIMER  		(7u)			// Low Power Interrupt Timer 0 Channel 0 IRQ for 200ms timing

#endif /* SYSTEM_CONFIG_H_ */
