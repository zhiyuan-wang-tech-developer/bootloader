/*
 * pc_communication.h
 *
 *  Created on: Oct 23, 2018
 *      Author: dynatron2018
 */

#ifndef PC_COMMUNICATION_H_
#define PC_COMMUNICATION_H_

#include "stdint.h"
#include "stdbool.h"

#define DATA_PACKET_LENGTH			255u
#define NACK_DATA_PACKET_LENGTH		5u
#define  ACK_DATA_PACKET_LENGTH		4u


/*
 * Data Packet Structure
 */
typedef union
{
	uint8_t buffer[DATA_PACKET_LENGTH];			// data packet buffer
	struct
	{
		uint8_t header;			// packet header = 0x55
		uint8_t type;			// packet type / packet identifier = 0..255
		uint8_t size;			// packet size = total amount of bytes in a received data packet
		uint8_t command;		// PC command field
		uint8_t raw_data[250];	// raw data payload
		uint8_t checksum; 		// (header + type + size + raw_data[0...] + checksum) % 256 == 0
	} item;
} DATA_PACKET_t;


/*
 * MCU-to-PC No Acknowledge Data Packet
 */
typedef union
{
	uint8_t buffer[NACK_DATA_PACKET_LENGTH];
	struct
	{
		uint8_t header;
		uint8_t type;
		uint8_t size;
		uint8_t error_info;
		uint8_t checksum;
	} item;
} NACK_DATA_PACKET_t;


/*
 * MCU-to-PC Acknowledge Data Packet
 */
typedef union
{
	uint8_t buffer[ACK_DATA_PACKET_LENGTH];
	struct
	{
		uint8_t header;
		uint8_t type;
		uint8_t size;
		uint8_t checksum;
	} item;
} ACK_DATA_PACKET_t;

/*
 * The finite state set for PC-to-UART Receiver State Machine
 */
typedef enum
{
	READY_FOR_DATA_RX,						// Check if UART RX Module is busy or not.
	INITIATE_DATA_RX,						// Initiate UART RX process if the UART RX Module is not busy.
	FIND_RX_DATA_PACKET_HEADER,				// Find RX data packet header from UART RX Ring Buffer.
	CHECK_RX_DATA_PACKET_TYPE,				// Check if the data packet type is put data type (0x0b).
	CHECK_RX_DATA_PACKET_SIZE,				// Check the data packet size (69 Bytes or 5 Bytes).
	CHECK_RX_DATA_PACKET_CMD,				// Check the PC command (Write flash or Reset).
	EXTRACT_RX_DATA_PACKET,					// Extract the raw data and checksum.
	CHECK_RX_DATA_PACKET,					// Check if the extracted data packet is correct and execute the PC command.
	WRITE_RPOGRAM_TO_FLASH,					// Execute PC command to write flash.
	SEND_ACKNOWLEDGE_MSG,					// Send the acknowledge message to PC after writing a data packet to flash.
	UPDATE_FIRMWARE_STATUS,					// End the data packet download and flash write, update the firmware status.
	RESET_MCU,								// Execute PC command to reset MCU.
} UART_RECEIVER_STATE_t;

/*
 * UART Receive FIFO Ring Buffer Structure
 */
typedef struct
{
	uint16_t putByteIndex;
	uint16_t getByteIndex;
	uint16_t usedBytesCount;		// Count the number of bytes that are already in the ring buffer.
	uint16_t size;
	uint8_t * pRingBuffer;
} FIFO_RING_BUFFER_t;

// Public global variable
extern DATA_PACKET_t rx_data_packet;
extern bool isFirmwareDownloading;

// Public function prototype
void PC2UART_communication_init(void);
void PC2UART_receiver_run_test(void);

#endif /* PC_COMMUNICATION_H_ */
