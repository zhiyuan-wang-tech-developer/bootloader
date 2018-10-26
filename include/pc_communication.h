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

#define DATA_PACKET_LENGTH	36

/*
 * Data Packet Structure
 */
typedef union
{
	uint8_t buffer[DATA_PACKET_LENGTH];			// data packet buffer
	struct
	{
		uint8_t header;			// packet header = 0x55
		uint8_t type;			// packet type / packet identifier 0..255
		uint8_t size;			// packet size = total amount of bytes in a data packet
		uint8_t raw_data[32];	// 32-bytes raw data payload
		uint8_t checksum; 		// ( header + type + size + raw_data[0...31] + checksum ) % 256 == 0
	} item;
} DATA_PACKET_t;

/*
 * The finite state set for PC-to-UART Receiver State Machine
 */
typedef enum
{
	READY_FOR_DATA_RX,						// Check if UART RX Module is busy or not.
	INITIATE_DATA_RX,						// Initiate UART RX process if the UART RX Module is not busy.
	FIND_RX_DATA_PACKET_HEADER,				// Find Rx data packet header from UART RX Ring Buffer.
	CHECK_RX_DATA_PACKET_TYPE,
	CHECK_RX_DATA_PACKET_SIZE,
	EXTRACT_RX_DATA_PACKET,
	CHECK_RX_DATA_PACKET,					// Check the extracted data packet
	SEND_ACKNOWLEDGE_MSG					// Send the acknowledge message to PC
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


extern DATA_PACKET_t rx_data_packet;

void PC2UART_communication_init(void);
void PC2UART_receiver_run(void);

#endif /* PC_COMMUNICATION_H_ */
