/******************************************************************************
 * @brief   USB CDC Interface functions for the Dragonfly quadrotor UAV
 *
 * @license
 * Dragonfly FCB firmware to control the Dragonfly quadrotor UAV
 * Copyright (C) 2016  ÅF Technology South: Dragonfly Project
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 *****************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "usbd_cdc_if.h"

#include "fifo_buffer.h"
#include "com_cli.h"
#include "usbd_cdc.h"
#include "fcb_error.h"
#include "communication.h"

#include <string.h>
#include <stdio.h>
#include <stdio_ext.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "FreeRTOS_CLI.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define USB_COM_TX_BUFFER_SIZE          512
#define USB_COM_RX_BUFFER_SIZE          512

#define USB_COM_RX_TASK_PRIO          	1
#define USB_COM_TX_TASK_PRIO          	2 // Prioritize sending over receiving so that buffers are emptied faster

#define USB_COM_TX_QUEUE_ITEMS          16
#define USB_RX_MAX_SEM_COUNT			4

#define USB_COM_MAX_DELAY               1000 // [ms]

/* Private function prototypes -----------------------------------------------*/
static void InitUSBCom(void);
static int8_t CDCItfInit(void);
static int8_t CDCItfDeInit(void);
static int8_t CDCItfControl(uint8_t cmd, uint8_t* pbuf, uint16_t length);
static int8_t CDCItfReceive(uint8_t* rxData, uint32_t* rxDataLen);

static void USBComPortRXTask(void const *argument);
static void USBComPortTXTask(void const *argument);

/* Private variables ---------------------------------------------------------*/

/* USB handler declaration */
USBD_HandleTypeDef hUSBDDevice;

uint8_t USBCOMRxPacketBuffer[CDC_DATA_HS_OUT_PACKET_SIZE];

/* USB CDC Receive FIFO buffer */
uint8_t USBCOMRxBufferArray[USB_COM_RX_BUFFER_SIZE];
volatile FIFOBuffer_TypeDef USBCOMRxFIFOBuffer;

/* USB CDC Transmit FIFO buffer */
uint8_t USBCOMTxBufferArray[USB_COM_TX_BUFFER_SIZE];
volatile FIFOBuffer_TypeDef USBCOMTxFIFOBuffer;

USBD_CDC_ItfTypeDef USBD_CDC_fops = { CDCItfInit, CDCItfDeInit, CDCItfControl, CDCItfReceive };

USBD_CDC_LineCodingTypeDef LineCoding = { 115200, /* baud rate */
0x00, /* stop bits-1 */
0x00, /* parity - none */
0x08 /* nb. of data bits 8 */
};

/* USB RTOS variables */
xTaskHandle USBComPortRxTaskHandle;
xTaskHandle USBComPortTxTaskHandle;

xQueueHandle usbComTxQueue;

xSemaphoreHandle USBCOMRxDataSem;
xSemaphoreHandle USBCOMTxBufferMutex;
xSemaphoreHandle USBTxMutex;

/* Private functions ---------------------------------------------------------*/

/**
 * @brief  Initializes the USB peripheral for CDC communication (COM port)
 * @param  None.
 * @retval None.
 */
static void InitUSBCom(void) {
	/* Create CDC RX FIFO Buffer */
	FIFOBufferInit(&USBCOMRxFIFOBuffer, USBCOMRxBufferArray, sizeof(USBCOMRxBufferArray));

	/* Create CDC TX FIFO Buffer */
	FIFOBufferInit(&USBCOMTxFIFOBuffer, USBCOMTxBufferArray, sizeof(USBCOMTxBufferArray));

	/* Init Device Library */
	USBD_Init(&hUSBDDevice, &VCP_Desc, 0);

	/* Add Supported Class */
	USBD_RegisterClass(&hUSBDDevice, &USBD_CDC);

	/* Add CDC Interface Class */
	USBD_CDC_RegisterInterface(&hUSBDDevice, &USBD_CDC_fops);

	/* Start Device Process */
	USBD_Start(&hUSBDDevice);
}

/**
 * @brief  CDCItfInit callback
 *         Initializes the CDC media low layer
 * @param  None
 * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
 */
static int8_t CDCItfInit(void) {
	/*# Set CDC Buffers ####################################################### */
	USBD_CDC_SetRxBuffer(&hUSBDDevice, USBCOMRxPacketBuffer);

	return (USBD_OK);
}

/**
 * @brief  CDCItfDeInit callback
 *         DeInitializes the CDC media low layer
 * @param  None
 * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
 */
static int8_t CDCItfDeInit(void) {
	return (USBD_OK);
}

/**
 * @brief  CDCItfControl callback
 *         Manage the CDC class requests
 * @param  Cmd: Command code
 * @param  Buf: Buffer containing command data (request parameters)
 * @param  Len: Number of data to be sent (in bytes)
 * @retval Result of the opeartion: USBD_OK if all operations are OK else USBD_FAIL
 */
static int8_t CDCItfControl(uint8_t cmd, uint8_t* pbuf, uint16_t length) {
	(void) length;

	switch (cmd) {
	case CDC_SEND_ENCAPSULATED_COMMAND:
		/* Add your code here */
		break;

	case CDC_GET_ENCAPSULATED_RESPONSE:
		/* Add your code here */
		break;

	case CDC_SET_COMM_FEATURE:
		/* Add your code here */
		break;

	case CDC_GET_COMM_FEATURE:
		/* Add your code here */
		break;

	case CDC_CLEAR_COMM_FEATURE:
		/* Add your code here */
		break;

	case CDC_SET_LINE_CODING:
		LineCoding.bitrate = (uint32_t) (pbuf[0] | (pbuf[1] << 8) |\
 (pbuf[2] << 16) | (pbuf[3] << 24));
		LineCoding.format = pbuf[4];
		LineCoding.paritytype = pbuf[5];
		LineCoding.datatype = pbuf[6];
		/* Add your code here */
		break;

	case CDC_GET_LINE_CODING:
		pbuf[0] = (uint8_t) (LineCoding.bitrate);
		pbuf[1] = (uint8_t) (LineCoding.bitrate >> 8);
		pbuf[2] = (uint8_t) (LineCoding.bitrate >> 16);
		pbuf[3] = (uint8_t) (LineCoding.bitrate >> 24);
		pbuf[4] = LineCoding.format;
		pbuf[5] = LineCoding.paritytype;
		pbuf[6] = LineCoding.datatype;

		/* Add your code here */
		break;

	case CDC_SET_CONTROL_LINE_STATE:
		/* Add your code here */
		break;

	case CDC_SEND_BREAK:
		/* Add your code here */
		break;

	default:
		break;
	}

	return USBD_OK;
}

/**
 * @brief  USB CDC receive callback. Data received over USB OUT endpoint sent over
 *         CDC interface through this function. Function called from ISR.
 * @param  rxData: Buffer of data
 * @param  rxDataLen: Number of data received (in bytes)
 * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
 */
static int8_t CDCItfReceive(uint8_t* rxData, uint32_t* rxDataLen) {
	uint8_t result = USBD_OK;
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

	if (hUSBDDevice.dev_state == USBD_STATE_CONFIGURED) {
		result = USBD_CDC_ReceivePacket(&hUSBDDevice);
		if (result == USBD_OK) {
			if (FIFOBufferPutData(&USBCOMRxFIFOBuffer, rxData, *rxDataLen)) {
				/* # Signal RX task that new USB CDC data has arrived #### */
				xSemaphoreGiveFromISR(USBCOMRxDataSem, &xHigherPriorityTaskWoken);
				portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
			} else {
				result = USBD_FAIL;
			}
		}
	} else {
		result = USBD_FAIL;
	}

	return result;
}

/**
 * @brief  Task code handles the USB Com Port Rx communication
 * @param  argument : Unused parameter
 * @retval None
 */
static void USBComPortRXTask(void const *argument) {
	(void) argument;

	uint16_t i = 0;
	uint16_t datalen = 0;
	portBASE_TYPE moreDataToFollow;
	ErrorStatus bufferStatus = SUCCESS;
	uint8_t getByte;
	static uint8_t cliInBuffer[MAX_CLI_COMMAND_SIZE];
	static uint8_t cliOutBuffer[MAX_CLI_OUTPUT_SIZE];

	/* Init USB communication */
	InitUSBCom();

	for (;;) {
		bufferStatus = SUCCESS;
		/* Wait forever for incoming data over USB by pending on the USB Rx semaphore */
		if (pdPASS == xSemaphoreTake(USBCOMRxDataSem, portMAX_DELAY)) {
			// Read out the FIFO buffer
			getByte = 0;
			while (bufferStatus == SUCCESS && i < MAX_CLI_COMMAND_SIZE && ((char)getByte) != '\r') {
				bufferStatus = FIFOBufferGetByte(&USBCOMRxFIFOBuffer, &getByte);

				if (bufferStatus == SUCCESS) {
					cliInBuffer[i] = getByte;
				}

				i++;
			}

			// End of command assumed found ('\r')
			if (((char)getByte) == '\r') {
				uint16_t k = 0;
				while((((char)cliInBuffer[k]) == ' ' || ((char)cliInBuffer[k]) == '\n' || ((char)cliInBuffer[k]) == '\r') && k < i-1) {
					k++;
				}

				TakeCLIMutex();
				do {
					/* Send the command string to the command interpreter. Any output generated
					 * by the command interpreter will be placed in the cliOutBuffer buffer. */
				    moreDataToFollow = CLIParser(&(cliInBuffer[k]), cliOutBuffer, &datalen);
				    if(datalen > 0) {
				        USBComSendData(cliOutBuffer, datalen);
				    } else {
				        USBComSendString((char*) cliOutBuffer);
				    }

				} while (moreDataToFollow != pdFALSE);
				GiveCLIMutex();

				i = 0;
				memset(cliInBuffer, 0x00, sizeof(cliInBuffer));
			}

			// If rxTempBuffer full without found command
			if (i >= MAX_CLI_COMMAND_SIZE) {
				i = 0;
				memset(cliInBuffer, 0x00, sizeof(cliInBuffer));
			}
		}
	}
}

/**
 * @brief  Task code handles the USB Com Port Tx communication
 * @param  argument : Unused parameter
 * @retval None
 */
static void USBComPortTXTask(void const *argument) {
	(void) argument;

	static ComTxQueueItem_TypeDef CompPortTxQueueItem;
	uint8_t* txDataPtr;
	uint16_t tmpSize;

	for (;;) {
		/* Wait forever for incoming data over USB by pending on the USB Tx queue */
		if (pdPASS == xQueueReceive(usbComTxQueue, &CompPortTxQueueItem, portMAX_DELAY)) {
			if (xSemaphoreTake(USBTxMutex, USB_COM_MAX_DELAY) == pdPASS) { // Pend on USB mutex while previous transmission is in progress
				/* Take the buffer mutex (if it has one) */
				if ((*CompPortTxQueueItem.BufferMutex == NULL
						|| xSemaphoreTake(*CompPortTxQueueItem.BufferMutex, portMAX_DELAY) == pdPASS)) {

					switch (CompPortTxQueueItem.bufferType) {
					case ARRAY_BUFFER:
						// Tx buffer is just a good ol' array of data
						CDCTransmitFS((uint8_t*) CompPortTxQueueItem.bufferPtr, CompPortTxQueueItem.dataSize);
						break;
					case FIFO_BUFFER:
						// Tx buffer is a FIFO ring buffer that wraps around its zero index
						tmpSize = FIFOBufferGetData((volatile FIFOBuffer_TypeDef*) CompPortTxQueueItem.bufferPtr,
								&txDataPtr, CompPortTxQueueItem.dataSize);
						CDCTransmitFS(txDataPtr, tmpSize);

						// If not all data received from buffer (due to FIFO wrap-around), get the rest
						if (tmpSize < CompPortTxQueueItem.dataSize) {
							tmpSize = FIFOBufferGetData((volatile FIFOBuffer_TypeDef*) CompPortTxQueueItem.bufferPtr,
									&txDataPtr, CompPortTxQueueItem.dataSize - tmpSize);
							CDCTransmitFS(txDataPtr, tmpSize);
						}
						break;
					default:
						/* Unspecified buffer type, indicate error */
						ErrorHandler();
						break;
					}
					if (*CompPortTxQueueItem.BufferMutex != NULL) {
						xSemaphoreGive(*CompPortTxQueueItem.BufferMutex); // Give buffer mutex
					}
				}
				xSemaphoreGive(USBTxMutex); // Give USB mutex when transmission completed
			}
		}
	}
}

/* Exported functions --------------------------------------------------------*/

/**
 * @brief  Data transmit over USB IN endpoint sent over CDC interface through this function. The data is entered into an output FIFO buffer which is then entered is
 *         then referred to into a queue. Both the FIFO buffer and the queue are protected by semaphore which needs to pended on.
 * @param  Buf: Buffer of data
 * @param  Len: Number of data received (in bytes)
 * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
 */
USBD_StatusTypeDef CDCTransmitFS(uint8_t* data, uint16_t size) {
	uint8_t result = USBD_OK;
	static uint16_t timeoutCnt = UINT16_MAX;
	USBD_CDC_HandleTypeDef *hCDC = hUSBDDevice.pClassData;

	if (hUSBDDevice.dev_state == USBD_STATE_CONFIGURED) {
		USBD_CDC_SetTxBuffer(&hUSBDDevice, data, size);
		while (((result = USBD_CDC_TransmitPacket(&hUSBDDevice)) == USBD_BUSY) && timeoutCnt != 0) {
			timeoutCnt--;
		} // Wait while busy with previous transmission
		if(timeoutCnt == 0)
			hCDC->TxState = 0;	// Reset the USB transfer state
		timeoutCnt = UINT16_MAX;
		if (size % CDC_DATA_FS_MAX_PACKET_SIZE == 0) {
			/*
			 * According to the USB specification, a packet size of 64 bytes (CDC_DATA_FS_MAX_PACKET_SIZE)
			 * gets held at the USB host until the next packet is sent.  This is because a
			 * packet of maximum size is considered to be part of a longer chunk of data, and
			 * the host waits for all data to arrive (ie, waits for a packet < max packet size).
			 * To flush a packet of exactly max packet size, we need to send a zero-size packet or
			 * short packet of less than CDC_DATA_FS_MAX_PACKET_SIZE.
			 * See eg http://www.cypress.com/?id=4&rID=92719
			 * */
			USBD_CDC_SetTxBuffer(&hUSBDDevice, NULL, 0); // Zero-length packet (ZLP)
			while (((result = USBD_CDC_TransmitPacket(&hUSBDDevice)) == USBD_BUSY) && timeoutCnt != 0) {
				timeoutCnt--;
			} // Wait while busy with previous transmission
			if(timeoutCnt == 0)
				hCDC->TxState = 0;	// Reset the USB transfer state
			timeoutCnt = UINT16_MAX;
		}
	} else {
		result = USBD_FAIL; // USB not connected and/or configured
	}
	return result;
}

/**
 * @brief  Send a string over the USB IN endpoint CDC com port interface.
 * @param  sendString : Reference to the string to be sent
 * @param  maxMutexWaitTicks : Max ticks to wait for the FIFO buffer mutex
 * @param  maxQueueWaitTicks : Max ticks to wait for the queue
 * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
 */
USBD_StatusTypeDef USBComSendString(const char* sendString) {
	return USBComSendData((uint8_t*) sendString, strlen(sendString));
}

/**
 * @brief  Send data over the USB IN endpoint CDC com port interface
 * @param  sendData : Reference to the data to be sent
 * @param  sendDataSize : Size of data to be sent
 * @param  maxMutexWaitTicks : Max ticks to wait for the FIFO buffer mutex
 * @param  maxQueueWaitTicks : Max ticks to wait for the queue
 * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
 */
USBD_StatusTypeDef USBComSendData(const uint8_t* sendData, const uint16_t sendDataSize) {
    ComTxQueueItem_TypeDef CompPortTxQueueItem;
	USBD_StatusTypeDef result = USBD_OK;

	if (xSemaphoreTake(USBCOMTxBufferMutex, USB_COM_MAX_DELAY) == pdPASS) {
		// Mutex obtained - access the shared resource
		if (FIFOBufferPutData(&USBCOMTxFIFOBuffer, sendData, sendDataSize)) {
			CompPortTxQueueItem.bufferType = FIFO_BUFFER;
			CompPortTxQueueItem.bufferPtr = (void*) &USBCOMTxFIFOBuffer;
			CompPortTxQueueItem.BufferMutex = &USBCOMTxBufferMutex;
			CompPortTxQueueItem.dataSize = sendDataSize;

			// The Tx Queue needs to be accessed in critical region since we don't want items from the same FIFO entering it in the wrong order!
			if (xQueueSend(usbComTxQueue, &CompPortTxQueueItem, USB_COM_MAX_DELAY) != pdPASS) {
				// Queue full, delete data from FIFO
				FIFOBufferDeleteLastEnteredBytes(&USBCOMTxFIFOBuffer, sendDataSize);
				result = USBD_FAIL;
			}
		} else {
			result = USBD_FAIL;
		}

		xSemaphoreGive(USBCOMTxBufferMutex); // We have finished accessing the shared resource. Release the mutex.
	} else {
		result = USBD_FAIL;
	}

	return result;
}

/**
 * @brief  Creates tasks used for USB communication
 * @param  None
 * @retval None
 */
void CreateUSBComTasks(void) {
	/* USB Virtual Com Port Rx handler task creation
	 * Task function pointer: USBComPortRXTask
	 * Task name: USB_COM_RX
	 * Stack depth: 3*configMINIMAL_STACK_SIZE
	 * Parameter: NULL
	 * Priority: USB_COM_RX_TASK_PRIO (0 to configMAX_PRIORITIES-1 possible)
	 * Handle: USBComPortRxTaskHandle
	 * */
	if (pdPASS
			!= xTaskCreate((pdTASK_CODE )USBComPortRXTask, (signed portCHAR*)"USB_COM_RX", 3*configMINIMAL_STACK_SIZE,
					NULL, USB_COM_RX_TASK_PRIO, &USBComPortRxTaskHandle)) {
		ErrorHandler();
	}

	/* USB Virtual Com Port Tx handler task creation
	 * Task function pointer: USBComPortTXTask
	 * Task name: USB_COM_TX
	 * Stack depth: configMINIMAL_STACK_SIZE
	 * Parameter: NULL
	 * Priority: USB_COM_TX_THREAD_PRIO (0 to configMAX_PRIORITIES-1 possible)
	 * Handle: USBComPortTxTaskHandle
	 * */
	if (pdPASS
			!= xTaskCreate((pdTASK_CODE )USBComPortTXTask, (signed portCHAR*)"USB_COM_TX", 1*configMINIMAL_STACK_SIZE,
					NULL, USB_COM_TX_TASK_PRIO, &USBComPortTxTaskHandle)) {
		ErrorHandler();
	}
}

/**
 * @brief  Creates RTOS queues used for USB communication
 * @param  None
 * @retval None
 */
void CreateUSBComQueues(void) {
	/* # Create queue for outgoing data ####################################### */
	usbComTxQueue = xQueueCreate(USB_COM_TX_QUEUE_ITEMS, sizeof(ComTxQueueItem_TypeDef));

	/* We want this queue to be viewable in a RTOS kernel aware debugger, so register it. */
	vQueueAddToRegistry(usbComTxQueue, (signed portCHAR*) "usbComTxQueue");
}

/**
 * @brief  Creates semaphores used for USB communication
 * @param  None
 * @retval None
 */
void CreateUSBComSemaphores(void) {
	USBCOMRxDataSem = xSemaphoreCreateCounting(USB_RX_MAX_SEM_COUNT, 0);
	if (USBCOMRxDataSem == NULL) {
		ErrorHandler();
	}

	USBCOMTxBufferMutex = xSemaphoreCreateMutex();
	if (USBCOMTxBufferMutex == NULL) {
		ErrorHandler();
	}

	/* Create mutex to protect USB CDC class output. The semaphore is taken when device is
	 * transmitting (USB CDC busy) and given when transfer has completed. */
	USBTxMutex = xSemaphoreCreateMutex();
	if (USBTxMutex == NULL) {
		ErrorHandler();
	}
}

/**
 * @}
 */

/**
 * @}
 */

/*****END OF FILE****/
