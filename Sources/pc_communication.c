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

#define UART_RX_RING_BUFFER_SIZE	256

// Acknowledge message
#define ACKNOWLEDGE_MSG 	"Send acknowledge to PC! Checksum OK\r\n"
#define ERROR_MSG			"Send error to PC! Checksum Wrong\r\n"

#define ACK_CODE	0x10u	// Acknowledge response data packet type
#define ERR_CODE	0x11u	// No Acknowledge response data packet type

// No acknowledge response data packet error info
const uint8_t 	WriteFlashMemoryError 	= 120u;	// The writing of flash program memory has failed
const uint8_t	ChecksumError       	= 121u;
const uint8_t	TimeoutError 			= 122u;

DATA_PACKET_t rx_data_packet;

uint8_t uart_rx_buffer[UART_RX_RING_BUFFER_SIZE] = {0};
FIFO_RING_BUFFER_t uart_rx_ring_buffer = {
			.putByteIndex = 0,
			.getByteIndex = 0,
			.usedBytesCount = 0,
			.size = sizeof(uart_rx_buffer),
			.pRingBuffer = uart_rx_buffer
			};

UART_RECEIVER_STATE_t PC2UART_ReceiverStatus = READY_FOR_DATA_RX;

const uint8_t DataPacketHeader = 0x55u;
const uint8_t DataPacketType_PutData = 0x0Bu;
const uint8_t DataPacketSize = 68u; // 0x44u

// Function declaration for internal use
bool isRxDataPacketCorrect( DATA_PACKET_t * pDataPacket );
bool parseDataPacket( DATA_PACKET_t * pDataPacket );
void printDataPacket( DATA_PACKET_t * pDataPacket );
void calculateChecksum( DATA_PACKET_t * pDataPacket );

void SendAcknowledge(void);
void SendNoAcknowledge(uint8_t errorInfo);

bool FifoRingBuffer_IsEmpty(void);
bool FifoRingBuffer_IsFull(void);
bool FifoRingBuffer_PutByte(uint8_t InputByte);
bool FifoRingBuffer_GetByte(uint8_t * pOutputByte);
void handleRxByte(void *driverState, uart_event_t event, void *userData);

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
    LPUART_DRV_InstallRxCallback(INST_LPUART0, handleRxByte, NULL);
}

/*
 * Run PC to s32k144 MCU UART rx communication state machine
 */
void PC2UART_receiver_run_test(void)
{
//	static status_t uart_rx_status = 0;
	static uint32_t byteCount = 0;						// Count the number of data bytes that are read out of the ring buffer.
//	static uint8_t byteTotalNum = 0;					// Total number of bytes in a data packet
	static bool isDataPacketCorrect = false;			// Indicate if the received data packet is expected data packet.
	uint8_t rxByte = 0;

	switch (PC2UART_ReceiverStatus)
	{
		case READY_FOR_DATA_RX:
			if( lpuart0_State.isRxBusy )
			{
				// There is an active data reception. Abort reception and WAIT!
				LPUART_DRV_AbortReceivingData(INST_LPUART0); 	// Disable RX interrupt and clear isRxBusy flag
				PC2UART_ReceiverStatus = READY_FOR_DATA_RX;
			}
			else
			{
				// UART RX module is not busy now. START data reception!
				PC2UART_ReceiverStatus = INITIATE_DATA_RX;
			}
			break;

		case INITIATE_DATA_RX:
			// Call non-blocking receive function to initiate the data reception process.
			LPUART_DRV_ReceiveData(INST_LPUART0, NULL, 0u);		// Enable RX Interrupt
			// Immediately return after the non-blocking receive data function is called.
			PC2UART_ReceiverStatus = FIND_RX_DATA_PACKET_HEADER;
			break;

		case FIND_RX_DATA_PACKET_HEADER:
			if( FifoRingBuffer_IsEmpty() )
			{
				// No rx byte in the FIFO Ring Buffer.
				// Wait until there is at least one rx byte.
				PC2UART_ReceiverStatus = FIND_RX_DATA_PACKET_HEADER;
			}
			else
			{
				// FIFO Ring Buffer has at least one byte.
				FifoRingBuffer_GetByte(&rxByte);
				// Find rx data packet header
				if( rxByte == DataPacketHeader )
				{
					// The data packet header is found. Next to check data packet type
					PC2UART_ReceiverStatus = CHECK_RX_DATA_PACKET_TYPE;
					// Clear the rx data packet.
					memset(rx_data_packet.buffer, 0u, sizeof(rx_data_packet.buffer));
					rx_data_packet.item.header = rxByte;
				}
				else
				{
					// The data packet header is not found. Continue to find the header.
					PC2UART_ReceiverStatus = FIND_RX_DATA_PACKET_HEADER;
				}
			}
			break;

		case CHECK_RX_DATA_PACKET_TYPE:
			if( FifoRingBuffer_IsEmpty() )
			{
				// No rx byte in the FIFO Ring Buffer.
				// Wait until there is at least one rx byte.
				PC2UART_ReceiverStatus = CHECK_RX_DATA_PACKET_TYPE;
			}
			else
			{
				// FIFO Ring Buffer has at least one byte.
				FifoRingBuffer_GetByte(&rxByte);
				// Check rx data packet type.
				if( rxByte == DataPacketType_PutData )
				{
					// The data packet type is correct. Next to check data packet size.
					PC2UART_ReceiverStatus = CHECK_RX_DATA_PACKET_SIZE;
					rx_data_packet.item.type = rxByte;
				}
				else
				{
					// The data packet type is wrong. Restart to find the header.
					PC2UART_ReceiverStatus = FIND_RX_DATA_PACKET_HEADER;
				}
			}
			break;

		case CHECK_RX_DATA_PACKET_SIZE:
			if( FifoRingBuffer_IsEmpty() )
			{
				// No RX byte in the FIFO Ring Buffer.
				// Wait until there is at least one RX byte.
				PC2UART_ReceiverStatus = CHECK_RX_DATA_PACKET_SIZE;
			}
			else
			{
				// FIFO Ring Buffer has at least one byte.
				FifoRingBuffer_GetByte(&rxByte);
				// Check RX data packet size.
				if( (rxByte >= 4u) && (rxByte <= 255u) )
				{
					// The data packet size is correct. Next to receive data payload.
					PC2UART_ReceiverStatus = EXTRACT_RX_DATA_PACKET;
					rx_data_packet.item.size = rxByte;
				}
				else
				{
					// The data packet size is wrong. Restart to find the header.
					PC2UART_ReceiverStatus = FIND_RX_DATA_PACKET_HEADER;
				}
			}
			break;

		case EXTRACT_RX_DATA_PACKET:
			if( FifoRingBuffer_IsEmpty() && (byteCount < (rx_data_packet.item.size - 3u)) )		// Now exclude the header, type and size
			{
				PC2UART_ReceiverStatus = EXTRACT_RX_DATA_PACKET;
			}
			else
			{
				if( (byteCount >= 0u) && (byteCount < (rx_data_packet.item.size - 4u)) )
				{
					// Read data payload
					FifoRingBuffer_GetByte(&rxByte);
					rx_data_packet.item.raw_data[byteCount++] = rxByte;
					PC2UART_ReceiverStatus = EXTRACT_RX_DATA_PACKET;
				}
				else if( byteCount == (rx_data_packet.item.size - 4u) )
				{
					// Read checksum
					FifoRingBuffer_GetByte(&rxByte);
					rx_data_packet.item.checksum = rxByte;
					byteCount = 0;
					PC2UART_ReceiverStatus = CHECK_RX_DATA_PACKET;
				}
				else
				{
					// byteCount error and restart to find the header
					PC2UART_ReceiverStatus = FIND_RX_DATA_PACKET_HEADER;
				}
			}
			break;

		case CHECK_RX_DATA_PACKET:
			if( parseDataPacket(&rx_data_packet) )
			{
				isDataPacketCorrect = true;
			}
			else
			{
				isDataPacketCorrect = false;
			}
			PC2UART_ReceiverStatus = SEND_ACKNOWLEDGE_MSG;
//			printDataPacket(&rx_data_packet);
			break;

		case SEND_ACKNOWLEDGE_MSG:
			if( isDataPacketCorrect )
			{
				printf("correct packet\r\n");
				SendAcknowledge();
//				LPUART_DRV_SendDataPolling(INST_LPUART0, (uint8_t *)ACKNOWLEDGE_MSG, strlen(ACKNOWLEDGE_MSG));
			}
			else
			{
				printf("incorrect packet\r\n");
//				calculateChecksum(&rx_data_packet);
				SendNoAcknowledge(ChecksumError);
//				LPUART_DRV_SendDataPolling(INST_LPUART0, (uint8_t *)ERROR_MSG, strlen(ERROR_MSG));
			}
			PC2UART_ReceiverStatus = FIND_RX_DATA_PACKET_HEADER;
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
	sum %= 256u;
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
	// One data packet contains at least header, type, size, checksum.
	if( pDataPacket->item.size < 4u )
	{
		return false;
	}
	// Test the checksum
	if( !isRxDataPacketCorrect(pDataPacket) )
	{
		return false;
	}

	if( pDataPacket->item.size == 5u )
	{
		printDataPacket(&rx_data_packet);
	}
	return true;
}

void printDataPacket( DATA_PACKET_t * pDataPacket )
{
	uint8_t i = 0;
	printf("Data Packet:");
	for( i = 0; i < pDataPacket->item.size - 1u; i++ )
	{
		printf(" %02x", pDataPacket->buffer[i]);
	}
	printf(" %02x\r\n", pDataPacket->item.checksum);
}

void calculateChecksum( DATA_PACKET_t * pDataPacket )
{
	uint8_t i = 0;
	uint8_t checksum = 0;
	for( i = 0; i < pDataPacket->item.size - 1u; i++ )
	{
		checksum += pDataPacket->buffer[i];
	}
	checksum = 256u - checksum;
	printf("Correct checksum: %02x\r\n", checksum);
}

// Send Acknowledge back to the PC
void SendAcknowledge(void)
{
	ACK_DATA_PACKET_t ack_data_packet;
	// Clear acknowledge data packet
	memset(ack_data_packet.buffer, 0, sizeof(ack_data_packet.buffer));
	ack_data_packet.item.header = DataPacketHeader;
	ack_data_packet.item.type = ACK_CODE;
	ack_data_packet.item.size = ACK_DATA_PACKET_LENGTH;
	// Calculate the checksum
	uint8_t checksum = 0u;
	uint8_t i = 0;
	for( i = ack_data_packet.item.size - 2; i != 255; i-- )
	{
		checksum -= ack_data_packet.buffer[i];
	}
	ack_data_packet.item.checksum = checksum;
	LPUART_DRV_SendDataPolling(INST_LPUART0, ack_data_packet.buffer, sizeof(ack_data_packet.buffer));
}

// Send No Acknowledge back to the PC
void SendNoAcknowledge(uint8_t errorInfo)
{
	NACK_DATA_PACKET_t nack_data_packet;
	// Clear NO acknowledge data packet
	memset(nack_data_packet.buffer, 0, sizeof(nack_data_packet.buffer));
	nack_data_packet.item.header = DataPacketHeader;
	nack_data_packet.item.type = ERR_CODE;
	nack_data_packet.item.size = NACK_DATA_PACKET_LENGTH;
	nack_data_packet.item.error_info = errorInfo;
	// Calculate the checksum
	uint8_t checksum = 0u;
	uint8_t i = 0;
	for( i = nack_data_packet.item.size - 2u; i != 255; i-- )
	{
		checksum -= nack_data_packet.buffer[i];
	}
	nack_data_packet.item.checksum = checksum;
	LPUART_DRV_SendDataPolling(INST_LPUART0, nack_data_packet.buffer, sizeof(nack_data_packet.buffer));
}

/*
 * FIFO Buffer Operation Function
 */
// Check if the RX FIFO Ring Buffer is empty.
bool FifoRingBuffer_IsEmpty(void)
{
	if( uart_rx_ring_buffer.usedBytesCount == 0 )
	{
		return true;
	}
	else
	{
		return false;
	}
}

// Check if the RX FIFO Ring Buffer is full.
bool FifoRingBuffer_IsFull(void)
{
	if( uart_rx_ring_buffer.usedBytesCount == uart_rx_ring_buffer.size )
	{
		return true;
	}
	else
	{
		return false;
	}
}

// Put a byte into the RX FIFO Ring Buffer
bool FifoRingBuffer_PutByte(uint8_t InputByte)
{
	if(FifoRingBuffer_IsFull())
	{
		return false;
	}
	uart_rx_ring_buffer.pRingBuffer[uart_rx_ring_buffer.putByteIndex] = InputByte;
	uart_rx_ring_buffer.usedBytesCount++;
	uart_rx_ring_buffer.putByteIndex = (uart_rx_ring_buffer.putByteIndex + 1) % uart_rx_ring_buffer.size;
	return true;
}

// Get a byte from the RX FIFO Ring Buffer
bool FifoRingBuffer_GetByte(uint8_t * pOutputByte)
{
	if(pOutputByte == NULL)
	{
		return false;
	}
	if(FifoRingBuffer_IsEmpty())
	{
		return false;
	}
	*pOutputByte = uart_rx_ring_buffer.pRingBuffer[uart_rx_ring_buffer.getByteIndex];
	uart_rx_ring_buffer.usedBytesCount--;
	uart_rx_ring_buffer.getByteIndex = (uart_rx_ring_buffer.getByteIndex + 1) % uart_rx_ring_buffer.size;
	return true;
}

// UART Rx Callback for continuous byte by byte reception
void handleRxByte(void *driverState, uart_event_t event, void *userData)
{
	uint8_t rxByte = 0;
    /* Check the event type */
	if(event == UART_EVENT_RX_FULL)
	{
		rxByte = (uint8_t)LPUART0->DATA;
//		printf("call back rx: %c\r\n", rxByte);
		FifoRingBuffer_PutByte(rxByte);
	}
}
