/*
 * pc_communication.c
 *
 *  Created on: Oct 23, 2018
 *      Author: dynatron2018
 */

#include "pc_communication.h"
#include "bootloader.h"
#include "Cpu.h"
#include "stdio.h"
#include "string.h"

#define UART_RX_RING_BUFFER_SIZE	256

// Acknowledge message
//#define ACKNOWLEDGE_MSG 	"Send acknowledge to PC! Checksum OK\r\n"
//#define ERROR_MSG			"Send error to PC! Checksum Wrong\r\n"

// MCU to PC Acknowledge Data Packet Type
#define ACK_CODE	0x10u	// Acknowledge response data packet type
#define ERR_CODE	0x11u	// No Acknowledge response data packet type

#define LED_OFF		PINS_DRV_ClearPins(PTE, 1<<8)
#define LED_ON		PINS_DRV_SetPins(PTE, 1<<8)

// PC to MCU data packet command code
const uint8_t WriteFlashMemory = 0x01u;			// Write new program to MCU flash memory.
const uint8_t ResetOK		= 0x02u;			// Reset the MCU and set the firmware update flag after writing firmware to flash is successful.
const uint8_t ResetNotOK	= 0x03u;			// Reset the MCU and clear the firmware update flag after writing firmware to flash is unsuccessful.

// The error info in no acknowledge response data packet
const uint8_t 	WriteFlashMemoryError 	= 120u;		// The writing of flash program memory has failed
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

// The flag to indicate if the firmware is being downloaded.
bool isFirmwareDownloading = false;

const uint8_t DataPacketHeader = 0x55u;
const uint8_t DataPacketType_PutData = 0x0Bu;
const uint8_t DataPacketSize = 69u; // 0x45u  The

// Function declaration for internal use
bool isRxDataPacketCorrect( DATA_PACKET_t * pDataPacket );
bool checkDataPacket( DATA_PACKET_t * pDataPacket );
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
void PC2UART_receiver_run(void)
{
//	static status_t uart_rx_status = 0;
	static uint32_t byteCount = 0;						// Count the number of data bytes that are read out of the ring buffer.
	static bool isWriteSuccessful = false;				// Return value to indicate if the write operation is successful.
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
				// Make sure LED off.
				LED_OFF;
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
				if( (rxByte >= 5u) && (rxByte <= 255u) )
				{
					// The data packet size is correct. Next to receive command.
					PC2UART_ReceiverStatus = CHECK_RX_DATA_PACKET_CMD;
					rx_data_packet.item.size = rxByte;
				}
				else
				{
					// The data packet size is wrong. Restart to find the header.
					PC2UART_ReceiverStatus = FIND_RX_DATA_PACKET_HEADER;
				}
			}
			break;

		case CHECK_RX_DATA_PACKET_CMD:
			if( FifoRingBuffer_IsEmpty() )
			{
				// No RX byte in the FIFO Ring Buffer.
				// Wait until there is at least one RX byte.
				PC2UART_ReceiverStatus = CHECK_RX_DATA_PACKET_CMD;
			}
			else
			{
				// FIFO Ring Buffer has at least one byte.
				FifoRingBuffer_GetByte(&rxByte);
				// Check RX data packet command.
				if( (rxByte == WriteFlashMemory) ||
					(rxByte == ResetOK) ||
					(rxByte == ResetNotOK) )
				{
					// The data packet command is what we expect. Next to receive data payload and checksum.
					PC2UART_ReceiverStatus = EXTRACT_RX_DATA_PACKET;
					rx_data_packet.item.command = rxByte;
					// Set the flag to indicate that the firmware is being downloaded.
					if(isFirmwareDownloading == false)
					{
						isFirmwareDownloading = true;
						// Turn LED on to indicate the firmware download in progress.
						LED_ON;
					}
				}
				else
				{
					// The data packet command is not what we expect. Restart to find the header.
					PC2UART_ReceiverStatus = FIND_RX_DATA_PACKET_HEADER;
				}
			}
			break;

		case EXTRACT_RX_DATA_PACKET:
			if( FifoRingBuffer_IsEmpty() && (byteCount < (rx_data_packet.item.size - 4u)) )		// Now ignore the header, type, size and command
			{
				PC2UART_ReceiverStatus = EXTRACT_RX_DATA_PACKET;
			}
			else
			{
				if( rx_data_packet.item.size == 5u )
				{
					// Directly read checksum
					FifoRingBuffer_GetByte(&rxByte);
					rx_data_packet.item.checksum = rxByte;
					byteCount = 0;
					PC2UART_ReceiverStatus = CHECK_RX_DATA_PACKET;
				}
				else
				{
					if( (byteCount >= 0u) && (byteCount < (rx_data_packet.item.size - 5u)) )  // ignore the header, type, size, command
					{
						// Read data payload
						FifoRingBuffer_GetByte(&rxByte);
						rx_data_packet.item.raw_data[byteCount++] = rxByte;
						PC2UART_ReceiverStatus = EXTRACT_RX_DATA_PACKET;
					}
					else if( byteCount == (rx_data_packet.item.size - 5u) )
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
			}
			break;

		case CHECK_RX_DATA_PACKET:
			// Check checksum actually.
			if( checkDataPacket(&rx_data_packet) )
			{
				isDataPacketCorrect = true;
			}
			else
			{
				isDataPacketCorrect = false;
			}

//			printDataPacket(&rx_data_packet);
			// Check command to execute
			if( rx_data_packet.item.command == WriteFlashMemory )
			{
				/*
				 * No matter if the data packet is correct or not,
				 * we always write it into flash memory.
				 */
				PC2UART_ReceiverStatus = WRITE_RPOGRAM_TO_FLASH;
			}
			else if( (rx_data_packet.item.command == ResetOK) ||
					 (rx_data_packet.item.command == ResetNotOK) )
			{
				/*
				 * All data packet transfer has been finished after the reset command is received.
				 * You do not need to send acknowledge anymore.
				 */
				PC2UART_ReceiverStatus = UPDATE_FIRMWARE_STATUS;
			}
			else
			{
				// unrecognized PC command and restart to find the header
				PC2UART_ReceiverStatus = FIND_RX_DATA_PACKET_HEADER;
			}
			break;

		case WRITE_RPOGRAM_TO_FLASH:
			isWriteSuccessful = flash_auto_write_64bytes();
			PC2UART_ReceiverStatus = SEND_ACKNOWLEDGE_MSG;
			break;

		case SEND_ACKNOWLEDGE_MSG:
			if( isDataPacketCorrect && isWriteSuccessful )
			{
				/*
				 * The data packet checksum is correct and
				 * successfully written into the flash memory.
				 * Then, send OK acknowledge.
				 */
//				printf("Correct packet\r\n");
//				LPUART_DRV_SendDataPolling(INST_LPUART0, (uint8_t *)ACKNOWLEDGE_MSG, strlen(ACKNOWLEDGE_MSG));
				SendAcknowledge();
			}
			else if( (!isDataPacketCorrect) && isWriteSuccessful )
			{
				/*
				 * The data packet checksum is not correct.
				 * But, the data packet is successfully written into the flash memory.
				 * Then, send checksum error acknowledge.
				 */
//				printf("Error: rx data packet\r\n");
//				LPUART_DRV_SendDataPolling(INST_LPUART0, (uint8_t *)ERROR_MSG, strlen(ERROR_MSG));
//				calculateChecksum(&rx_data_packet);
				SendNoAcknowledge(ChecksumError);
			}
			else
			{
				/*
				 * reValue == false
				 * If it is failed to write the data packet into the flash memory,
				 * then send write flash memory error acknowledge.
				 */
//				printf("Error: flash write\r\n");
				SendNoAcknowledge(WriteFlashMemoryError);
			}
			PC2UART_ReceiverStatus = FIND_RX_DATA_PACKET_HEADER;
			break;

		case UPDATE_FIRMWARE_STATUS:
			// All data packet transfer has ended.

			// Calculate the size of the new firmware.
			new_firmware_status.newFirmwareSize = calculateNewFirmwareSize();

			// Calculate the checksum of the new firmware.
			uint32_t checksum = 0;
			if(true == calculateNewFirmwareChecksum(&checksum))
			{
				new_firmware_status.newFirmwareChecksum = checksum;
			}

			// Set the firmware update flag
			if(rx_data_packet.item.command == ResetOK)
			{
				// Successful in new firmware downloading.
//				printf("Success in new firmware download!\r\n");
				// New firmware is updated
				new_firmware_status.isNewFirmwareUpdated = 1u;
			}

			if(rx_data_packet.item.command == ResetNotOK)
			{
				// Failed in new firmware downloading.
//				printf("Failure in new firmware download!\r\n");
				// New firmware is not updated
				new_firmware_status.isNewFirmwareUpdated = 0u;
			}

			// Store the new firmware status into EEPROM for use in next restart.
			isWriteSuccessful = eeprom_write_new_firmware_status();

			PC2UART_ReceiverStatus = RESET_MCU;
			break;

		case RESET_MCU:
			// Disable UART module
			LPUART_DRV_Deinit(INST_LPUART0);
			// Clear the flag to indicate that the firmware download has ended.
			isFirmwareDownloading = false;
			// Turn off LED to indicate the end of the firmware downloading.
			LED_OFF;
			// Reset the PC to UART receiver status
			PC2UART_ReceiverStatus = READY_FOR_DATA_RX;
//			printf("System Reset...\r\n");
//			auto_ram_reset();
			auto_flash_reset();
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
	for(i = 0; i < (pDataPacket->item.size - 1u); i++)
	{
		sum += pDataPacket->buffer[i];
	}
	sum += pDataPacket->item.checksum;
	sum %= 256u;
	if( sum == 0 )
		return true;
	else
		return false;
}

/*
 * Check the data packet contents
 * @parameter:	pointer to the data packet
 * @return:		true if the data packet is what we expect
 * 				false if the data packet is not what we expect
 */
bool checkDataPacket( DATA_PACKET_t * pDataPacket )
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
	// One data packet contains at least header, type, size, command and checksum.
	if( pDataPacket->item.size < 5u )
	{
		return false;
	}

	// To print the reset command
	if( pDataPacket->item.size == 5u )
	{
		printDataPacket(&rx_data_packet);
	}

	// Check PC command
	if( (pDataPacket->item.command != WriteFlashMemory) &&
		(pDataPacket->item.command != ResetOK) &&
		(pDataPacket->item.command != ResetNotOK) )
	{
		// Unrecognized command
		return false;
	}

	// Test the checksum
	if( !isRxDataPacketCorrect(pDataPacket) )
	{
		return false;
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
	printf("Correct Checksum: %02x\r\n", checksum);
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
		/*
		 * Remove print function, otherwise the RX Overrun event will happen.
		 */
//		printf("call back rx: %c\r\n", rxByte);
		FifoRingBuffer_PutByte(rxByte);
	}
}
