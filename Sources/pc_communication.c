/*
 * pc_communication.c
 *
 *  Created on: Oct 23, 2018
 *      Author: dynatron2018
 */

#include "pc_communication.h"
#include "Cpu.h"
#include "stdio.h"
#include "string.h"

// Acknowledge message
#define ACKNOWLEDGE_MSG 	"Send acknowledge to PC!\r\n"
#define ERROR_MSG			"Send error to PC!\r\n"

DATA_PACKET_t rx_data_packet;

UART_RECEIVER_STATE_t PC2UART_ReceiverStatus = READY_FOR_DATA_RX;

const uint8_t DataPacketHeader = 0x55u;
const uint8_t DataPacketType_PutData = 0x0Bu;

// Function declaration for internal use
bool isRxDataPacketCorrect( DATA_PACKET_t * pDataPacket );

/*
 * Initialize the PC to s32k144 MCU UART communication
 */
void PC2UART_communication_init(void)
{
    LPUART_DRV_Init(INST_LPUART0, &lpuart0_State, &lpuart0_InitConfig0);
    INT_SYS_ClearPending(LPUART0_RxTx_IRQn);
    INT_SYS_SetPriority(LPUART0_RxTx_IRQn, 8);
//    uint8_t lpuart0_interrupt_priority = 0;
//    lpuart0_interrupt_priority = INT_SYS_GetPriority(LPUART0_RxTx_IRQn);
}

/*
 * Run PC to s32k144 MCU UART rx communication state machine
 */
void PC2UART_receiver_run(void)
{
	static status_t uart_rx_status = 0;
	static uint32_t remainingRxBytes = 0;				// The remaining number of bytes to be received
	static bool isDataPacketCorrect = false;			// Indicate if the received data packet is expected data packet.

	switch (PC2UART_ReceiverStatus)
	{
		case READY_FOR_DATA_RX:
			if( lpuart0_State.isRxBusy )
			{
				// There is an active data reception. WAIT!
				PC2UART_ReceiverStatus = READY_FOR_DATA_RX;
			}
			else
			{
				// UART rx module is not busy now. START data reception!
				PC2UART_ReceiverStatus = INITIATE_DATA_RX;
			}
			break;

		case INITIATE_DATA_RX:
			// Call non-blocking receive function to initiate the data reception process.
			LPUART_DRV_ReceiveData(INST_LPUART0, rx_data_packet.buffer, 15);
			// Immediately return after the non-blocking receive data function is called.
			PC2UART_ReceiverStatus = CHECK_UART_RX_COMPLETE;
			// Clear the rx data packet.
			memset(rx_data_packet.buffer, 0u, sizeof(rx_data_packet.buffer));
			break;

		case CHECK_UART_RX_COMPLETE:
			uart_rx_status = LPUART_DRV_GetReceiveStatus(INST_LPUART0, &remainingRxBytes);
			if( uart_rx_status == STATUS_SUCCESS )
			{
				// The data reception has been complete successfully!
				// Parse received data after data reception is complete.
				PC2UART_ReceiverStatus = PARSE_RX_DATA_PACKET;
			}
			else if( uart_rx_status == STATUS_BUSY )
			{
				// The data reception is still in progress!
				// Wait and Continue to check if data reception is complete.
				PC2UART_ReceiverStatus = CHECK_UART_RX_COMPLETE;
				// Print the number of bytes that still need to be received.
//				printf("Remaining Rx Bytes: %lu\r\n", remainingRxBytes);
			}
			else
			{
				// For other exceptional rx status, Reset the receiver state machine.
				PC2UART_ReceiverStatus = READY_FOR_DATA_RX;
			}
			break;

		case PARSE_RX_DATA_PACKET:
			if( 0 == strncmp((char *)rx_data_packet.buffer, "firmware update", 15) )
			{
				isDataPacketCorrect = true;
				printf("start to update firmware!\n");
			}
			else
			{
				isDataPacketCorrect = false;
				printf("rx data packet can not be parsed!\n");
			}
			PC2UART_ReceiverStatus = SEND_ACKNOWLEDGE_MSG;
			break;

		case SEND_ACKNOWLEDGE_MSG:
			if( isDataPacketCorrect )
			{
				LPUART_DRV_SendDataPolling(INST_LPUART0, (uint8_t *)ACKNOWLEDGE_MSG, strlen(ACKNOWLEDGE_MSG));
			}
			else
			{
				LPUART_DRV_SendDataPolling(INST_LPUART0, (uint8_t *)ERROR_MSG, strlen(ERROR_MSG));
			}
			PC2UART_ReceiverStatus = READY_FOR_DATA_RX;
			break;

		default:
			// If the receiver state machine happens to be in the default state, RESET the state machine.
			PC2UART_ReceiverStatus = READY_FOR_DATA_RX;
			break;
	}
}

/*
 * 	Check if the received data packet is correct.
 *  The checksum is calculated over the complete data packet.
 *  (header + type + size + raw_data[0..31] + checksum ) MOD 256 == 0
 *
 */
bool isRxDataPacketCorrect( DATA_PACKET_t * pDataPacket )
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

/*
 * Parse the data packet contents
 * @parameter:	pointer to the data packet
 * @return:		true if the data packet is what we expect
 * 				false if the data packet is not what we expect
 */
bool parseDataPacket( DATA_PACKET_t * pDataPacket )
{
	// Check data packet header
	if( pDataPacket->item.header != DataPacketHeader )
	{
		return false;
	}
	// Check data packet type
	if( pDataPacket->item.type != DataPacketType_PutData )
	{
		return false;
	}
	// Check data size
	if( pDataPacket->item.size != DATA_PACKET_LENGTH )
	{
		return false;
	}
	// Test the checksum
	if( !isRxDataPacketCorrect(pDataPacket) )
	{
		return false;
	}
	return true;
}
