/**
 ******************************************************************************
 * @file    usbd_cdc_if.h
 * @brief   USB CDC Interface header file
 ******************************************************************************
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USBD_CDC_IF_H
#define __USBD_CDC_IF_H

/* Includes ------------------------------------------------------------------*/
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_cdc.h"

#include "usbd_cdc.h"

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
USBD_StatusTypeDef CDCTransmitFS(uint8_t* data, uint16_t size);
USBD_StatusTypeDef USBComSendString(const char* sendString);
USBD_StatusTypeDef USBComSendData(const uint8_t* sendData, const uint16_t sendDataSize);
void CreateUSBComTasks(void);
void CreateUSBComQueues(void);
void CreateUSBComSemaphores(void);

#endif /* __USBD_CDC_IF_H */

/**
 * @}
 */

/**
 * @}
 */
/*****END OF FILE****/
