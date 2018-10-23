/*
 * pc_communication.c
 *
 *  Created on: Oct 23, 2018
 *      Author: dynatron2018
 */

#include "pc_communication.h"
#include "Cpu.h"

DATA_PACKET_t rx_data_packet;


/*
 *  The checksum is calculated over the complete data packet.
 *  (header + type + size + raw_data[0..31] + checksum ) MOD 256 == 0
 *
 */
bool isChecksumCorrect( DATA_PACKET_t * pDataPacket )
{
	uint8_t i = 0;
	uint8_t sum = 0;
	for(i = 0; i < sizeof(pDataPacket->buffer); i++)
	{
		sum += pDataPacket->buffer[i];
	}
	sum %= 256;
	if( sum == 0 )
		return true;
	else
		return false;
}

