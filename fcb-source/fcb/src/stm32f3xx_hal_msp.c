/**
 ******************************************************************************
 * @file    stm32f3xx_hal_msp.c
 * @author  Dragonfly
 *          Daniel Stenberg
 * @version V1.0.0
 * @date    2015-05-07
 * @brief   HAL MSP (MCU Specific Package) module. These functions are called
 *          from the HAL library.
 *
 @verbatim
 ===============================================================================
 ##### How to use this driver #####
 ===============================================================================
 [..]
 This file is generated automatically by MicroXplorer and eventually modified
 by the user

 @endverbatim
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT(c) 2014 STMicroelectronics</center></h2>
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. Neither the name of STMicroelectronics nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "stm32f3xx_hal.h"
#include "stm32f3xx.h"

#include "motor_control.h"
#include "receiver.h"
#include "state_estimation.h"
#include "uart.h"

#include "task_status.h"

/** @addtogroup STM32F3xx_HAL_Driver
 * @{
 */

/** @defgroup HAL_MSP HAL MSP module
 * @brief HAL MSP module.
 * @{
 */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Exported functions ---------------------------------------------------------*/

/** @defgroup HAL_MSP_Exported_Functions HAL MSP Exported Functions
 * @{
 */

/**
 * @brief  Initializes the Global MSP.
 * @retval None
 */
void HAL_MspInit(void) {
	/* NOTE : This function is generated automatically by MicroXplorer and eventually
	 modified by the user
	 */
}

/**
 * @brief  DeInitializes the Global MSP.
 * @retval None
 */
void HAL_MspDeInit(void) {
	/* NOTE : This function is generated automatically by MicroXplorer and eventually
	 modified by the user
	 */
}

/**
 * @brief  Initializes the PPP MSP.
 * @retval None
 */
void HAL_PPP_MspInit(void) {
	/* NOTE : This function is generated automatically by MicroXplorer and eventually
	 modified by the user
	 */
}

/**
 * @brief  DeInitializes the PPP MSP.
 * @retval None
 */
void HAL_PPP_MspDeInit(void) {
	/* NOTE : This function is generated automatically by MicroXplorer and eventually
	 modified by the user
	 */
}

/**
 * @brief CRC MSP Initialization
 * @param hcrc: CRC handle pointer
 * @retval None
 */
void HAL_CRC_MspInit(CRC_HandleTypeDef *hcrc) {
	(void) hcrc; // Avoid compile warning

	/* CRC Peripheral clock enable */
	__CRC_CLK_ENABLE();
}

/**
 * @brief CRC MSP De-Initialization
 * @param hcrc: CRC handle pointer
 * @retval None
 */
void HAL_CRC_MspDeInit(CRC_HandleTypeDef *hcrc) {
	(void) hcrc; // Avoid compile warning

	/* CRC Peripheral clock disable */
	__CRC_CLK_DISABLE();
}

/**
 * @brief TIM MSP Initialization
 *        This function configures the hardware resources used in this example:
 * @param htim: TIM handle pointer
 * @retval None
 */
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim) {

    if (htim->Instance == PRIMARY_RECEIVER_TIM) {
        /*##-1- Enable peripherals and GPIO Clocks #################################*/
        /* Primary Receiver TIM Peripheral clock enable */
        PRIMARY_RECEIVER_TIM_CLK_ENABLE();

        /*##-2- Configure the NVIC for PRIMARY_RECEIVER_TIM ########################*/
        HAL_NVIC_SetPriority(PRIMARY_RECEIVER_TIM_IRQn, PRIMARY_RECEIVER_TIM_IRQ_PREEMPT_PRIO,
        PRIMARY_RECEIVER_TIM_IRQ_SUB_PRIO);

        /* Enable the PRIMARY_RECEIVER_TIM global Interrupt */
        HAL_NVIC_EnableIRQ(PRIMARY_RECEIVER_TIM_IRQn);
    } else if (htim->Instance == AUX_RECEIVER_TIM) {
        /*##-1- Enable peripherals and GPIO Clocks #################################*/
        /* Aux Receiver TIM Peripheral clock enable */
        AUX_RECEIVER_TIM_CLK_ENABLE();

        /*##-2- Configure the NVIC for AUX_RECEIVER_TIM ############################*/
        HAL_NVIC_SetPriority(AUX_RECEIVER_TIM_IRQn, AUX_RECEIVER_TIM_IRQ_PREEMPT_PRIO, AUX_RECEIVER_TIM_IRQ_SUB_PRIO);

        /* Enable the AUX_RECEIVER_TIM global Interrupt */
        HAL_NVIC_EnableIRQ(AUX_RECEIVER_TIM_IRQn);
    } else if (htim->Instance == TASK_STATUS_TIM) {
        /*##-1- Enable peripherals and GPIO Clocks #################################*/
        /* TIM Task status clock enable */
        TASK_STATUS_TIM_CLK_ENABLE();

        /*##-2- Configure the NVIC for TASK_STATUS_TIM ############################*/
        HAL_NVIC_SetPriority(TASK_STATUS_TIM_IRQn, TASK_STATUS_TIM_IRQ_PREEMPT_PRIO, TASK_STATUS_TIM_IRQ_SUB_PRIO);

        /* Enable the TASK_STATUS_TIM global Interrupt */
        HAL_NVIC_EnableIRQ(TASK_STATUS_TIM_IRQn);
    } else if (htim->Instance == STATE_ESTIMATION_UPDATE_TIM) {
        /*##-1- Enable peripherals and GPIO Clocks #################################*/
        /* TIM State estimation update clock enable */
        STATE_ESTIMATION_UPDATE_TIM_CLK_ENABLE();

        /*##-2- Configure the NVIC for STATE_ESTIMATION_UPDATE_TIM ############################*/
        HAL_NVIC_SetPriority(STATE_ESTIMATION_UPDATE_TIM_IRQn, STATE_ESTIMATION_UPDATE_TIM_IRQ_PREEMPT_PRIO,
        STATE_ESTIMATION_UPDATE_TIM_IRQ_SUB_PRIO);

        /* Enable the STATE_ESTIMATION_UPDATE_TIM global Interrupt */
        HAL_NVIC_EnableIRQ(STATE_ESTIMATION_UPDATE_TIM_IRQn);
    }
}

/**
 * @brief TIM MSP Deinitialization
 * @param htim: TIM handle pointer
 * @retval None
 */
void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef *htim) {

    if(htim->Instance ==  PRIMARY_RECEIVER_TIM) {
        /* Primary receiver TIM Peripheral clock disable */
        PRIMARY_RECEIVER_TIM_CLK_DISABLE();
    } else if(htim->Instance == AUX_RECEIVER_TIM) {
        /* Aux receiver TIM Peripheral clock disable */
        AUX_RECEIVER_TIM_CLK_DISABLE();
    } else if(htim->Instance == TASK_STATUS_TIM) {
        /* Task status TIM Peripheral clock disable */
        TASK_STATUS_TIM_CLK_DISABLE();
    } else if(htim->Instance == STATE_ESTIMATION_UPDATE_TIM) {
        /* State estimation update TIM Peripheral clock disable */
        STATE_ESTIMATION_UPDATE_TIM_CLK_DISABLE();
    }
}

/**
 * @brief TIM PWM MSP Initialization
 * @param htim: TIM handle pointer
 * @retval None
 */
void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef *htim) {
	if (htim->Instance == TIM_MOTOR) {
		GPIO_InitTypeDef GPIO_InitStruct;
		/*##-1- Enable peripherals and GPIO Clocks #################################*/
		/* Motor TIM Peripheral clock enable */
		MOTOR_TIM_CLK_ENABLE();

		/* Enable Motor GPIO Channels Clock */
		MOTOR_TIM_CHANNEL_GPIO_PORT();

		/* Configure Motor GPIO Pins */
		/* Common configuration for all channels */
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_PULLUP;
		GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
		GPIO_InitStruct.Alternate = MOTOR_TIM_AF;

		/* Motor channel 1 pin */
		GPIO_InitStruct.Pin = MOTOR_GPIO_PIN_CHANNEL1;
		HAL_GPIO_Init(MOTOR_PIN_PORT, &GPIO_InitStruct);

		/* Motor channel 2 pin */
		GPIO_InitStruct.Pin = MOTOR_GPIO_PIN_CHANNEL2;
		HAL_GPIO_Init(MOTOR_PIN_PORT, &GPIO_InitStruct);

		/* Motor channel 3 pin */
		GPIO_InitStruct.Pin = MOTOR_GPIO_PIN_CHANNEL3;
		HAL_GPIO_Init(MOTOR_PIN_PORT, &GPIO_InitStruct);

		/* Motor channel 4 pin */
		GPIO_InitStruct.Pin = MOTOR_GPIO_PIN_CHANNEL4;
		HAL_GPIO_Init(MOTOR_PIN_PORT, &GPIO_InitStruct);
	}
}

/**
 * @brief TIM PWM MSP Initialization
 * @param htim: TIM handle pointer
 * @retval None
 */
void HAL_TIM_PWM_MspDeInit(TIM_HandleTypeDef *htim) {
	if (htim->Instance == TIM_MOTOR) {
		/* Motor TIM Peripheral clock disable */
		MOTOR_TIM_CLK_DISABLE();
	}
}

/**
 * @brief TIM IC MSP Initialization
 * @param htim: TIM handle pointer
 * @retval None
 */
void HAL_TIM_IC_MspInit(TIM_HandleTypeDef *htim) {
	if (htim->Instance == PRIMARY_RECEIVER_TIM) {
		GPIO_InitTypeDef GPIO_InitStruct;

		/*##-1- Enable peripherals and GPIO Clocks #################################*/
		/* Primary Receiver TIM Peripheral clock enable */
		PRIMARY_RECEIVER_TIM_CLK_ENABLE();

		/* Enable GPIO channels Clock */
		PRIMARY_RECEIVER_TIM_CHANNEL_GPIO_PORT();

		/* Configure  channels for Alternate function, push-pull and 100MHz speed */
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_PULLUP;
		GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
		GPIO_InitStruct.Alternate = PRIMARY_RECEIVER_TIM_AF;

		/* Primary Receiver Channel 1 pin */
		GPIO_InitStruct.Pin = PRIMARY_RECEIVER_PIN_CHANNEL1;
		HAL_GPIO_Init(PRIMARY_RECEIVER_TIM_PIN_PORT, &GPIO_InitStruct);

		/* Primary Receiver Channel 2 pin */
		GPIO_InitStruct.Pin = PRIMARY_RECEIVER_PIN_CHANNEL2;
		HAL_GPIO_Init(PRIMARY_RECEIVER_TIM_PIN_PORT, &GPIO_InitStruct);

		/* Primary Receiver Channel 3 pin */
		GPIO_InitStruct.Pin = PRIMARY_RECEIVER_PIN_CHANNEL3;
		HAL_GPIO_Init(PRIMARY_RECEIVER_TIM_PIN_PORT, &GPIO_InitStruct);

		/* Primary Receiver Channel 4 pin */
		GPIO_InitStruct.Pin = PRIMARY_RECEIVER_PIN_CHANNEL4;
		HAL_GPIO_Init(PRIMARY_RECEIVER_TIM_PIN_PORT, &GPIO_InitStruct);

		/*##-2- Configure the NVIC for PRIMARY_RECEIVER_TIM ########################*/
		HAL_NVIC_SetPriority(PRIMARY_RECEIVER_TIM_IRQn, PRIMARY_RECEIVER_TIM_IRQ_PREEMPT_PRIO,
				PRIMARY_RECEIVER_TIM_IRQ_SUB_PRIO);

		/* Enable the PRIMARY_RECEIVER_TIM global Interrupt */
		HAL_NVIC_EnableIRQ(PRIMARY_RECEIVER_TIM_IRQn);
	} else if (htim->Instance == AUX_RECEIVER_TIM) {
		GPIO_InitTypeDef GPIO_InitStruct;

		/*##-1- Enable peripherals and GPIO Clocks #################################*/
		/* Aux Receiver TIM Peripheral clock enable */
		AUX_RECEIVER_TIM_CLK_ENABLE();

		/* Enable GPIO channels Clock */
		AUX_RECEIVER_TIM_CHANNEL_GPIO_PORT();

		/* Configure channels for Alternate function, push-pull and 100MHz speed */
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_PULLUP;
		GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
		GPIO_InitStruct.Alternate = AUX_RECEIVER_TIM_AF;

		/* Aux Receiver Channel 1 pin */
		GPIO_InitStruct.Pin = AUX_RECEIVER_PIN_CHANNEL1;
		HAL_GPIO_Init(AUX_RECEIVER_TIM_PIN_PORT, &GPIO_InitStruct);

		/* Aux Receiver Channel 2 pin */
		GPIO_InitStruct.Pin = AUX_RECEIVER_PIN_CHANNEL2;
		HAL_GPIO_Init(AUX_RECEIVER_TIM_PIN_PORT, &GPIO_InitStruct);

		/*##-2- Configure the NVIC for AUX_RECEIVER_TIM ############################*/
		HAL_NVIC_SetPriority(AUX_RECEIVER_TIM_IRQn, AUX_RECEIVER_TIM_IRQ_PREEMPT_PRIO, AUX_RECEIVER_TIM_IRQ_SUB_PRIO);

		/* Enable the AUX_RECEIVER_TIM global Interrupt */
		HAL_NVIC_EnableIRQ(AUX_RECEIVER_TIM_IRQn);
	}
}

/**
 * @brief TIM IC MSP Deinitialization
 * @param htim: TIM handle pointer
 * @retval None
 */
void HAL_TIM_IC_MspDeInit(TIM_HandleTypeDef *htim) {
	if (htim->Instance == PRIMARY_RECEIVER_TIM) {
		/* Primary receiver TIM Peripheral clock disable */
		PRIMARY_RECEIVER_TIM_CLK_DISABLE();
	} else if (htim->Instance == AUX_RECEIVER_TIM) {
		/* Aux receiver TIM Peripheral clock disable */
		AUX_RECEIVER_TIM_CLK_DISABLE();
	}
}

/**
  * @brief UART MSP Initialization
  *        This function configures the hardware resources used in this example:
  *           - Peripheral's clock enable
  *           - Peripheral's GPIO Configuration
  *           - DMA configuration for transmission request by peripheral
  *           - NVIC configuration for DMA interrupt request enable
  * @param huart: UART handle pointer
  * @retval None
  */
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
  static DMA_HandleTypeDef hdma_tx;
  static DMA_HandleTypeDef hdma_rx;

  GPIO_InitTypeDef GPIO_InitStruct;

  /*##-1- Enable peripherals and GPIO Clocks #################################*/
  /* Enable GPIO TX/RX clock */
  UART_TX_GPIO_CLK_ENABLE();
  UART_RX_GPIO_CLK_ENABLE();

  /* Enable UART clock */
  UART_CLK_ENABLE();

  /* Enable DMA clock */
  UART_DMA_CLK_ENABLE();

  /*##-2- Configure peripheral GPIO ##########################################*/
  /* UART TX GPIO pin configuration  */
  GPIO_InitStruct.Pin       = UART_TX_PIN;
  GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull      = GPIO_PULLUP;
  GPIO_InitStruct.Speed     = GPIO_SPEED_HIGH;
  GPIO_InitStruct.Alternate = UART_TX_AF;

  HAL_GPIO_Init(UART_TX_GPIO_PORT, &GPIO_InitStruct);

  /* UART RX GPIO pin configuration  */
  GPIO_InitStruct.Pin = UART_RX_PIN;
  GPIO_InitStruct.Alternate = UART_RX_AF;

  HAL_GPIO_Init(UART_RX_GPIO_PORT, &GPIO_InitStruct);

  /*##-3- Configure the DMA channels ##########################################*/
  /* Configure the DMA handler for Transmission process */
  hdma_tx.Instance                 = UART_TX_DMA_STREAM;
  hdma_tx.Init.Direction           = DMA_MEMORY_TO_PERIPH;
  hdma_tx.Init.PeriphInc           = DMA_PINC_DISABLE;
  hdma_tx.Init.MemInc              = DMA_MINC_ENABLE;
  hdma_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
  hdma_tx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
  hdma_tx.Init.Mode                = DMA_NORMAL;
  hdma_tx.Init.Priority            = DMA_PRIORITY_LOW;

  HAL_DMA_Init(&hdma_tx);

  /* Associate the initialized DMA handle to the UART handle */
  __HAL_LINKDMA(huart, hdmatx, hdma_tx);

  /* Configure the DMA handler for reception process */
  hdma_rx.Instance                 = UART_RX_DMA_STREAM;
  hdma_rx.Init.Direction           = DMA_PERIPH_TO_MEMORY;
  hdma_rx.Init.PeriphInc           = DMA_PINC_DISABLE;
  hdma_rx.Init.MemInc              = DMA_MINC_DISABLE;
  hdma_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
  hdma_rx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
  hdma_rx.Init.Mode                = DMA_CIRCULAR;
  hdma_rx.Init.Priority            = DMA_PRIORITY_LOW;

  HAL_DMA_Init(&hdma_rx);

  /* Associate the initialized DMA handle to the the UART handle */
  __HAL_LINKDMA(huart, hdmarx, hdma_rx);

  /*##-4- Configure the NVIC for DMA #########################################*/
  /* NVIC configuration for DMA transfer complete interrupt (USARTx_TX) */
  HAL_NVIC_SetPriority(UART_DMA_TX_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 0);
  HAL_NVIC_EnableIRQ(UART_DMA_TX_IRQn);

  /* NVIC configuration for DMA transfer complete interrupt (USARTx_RX) */
  HAL_NVIC_SetPriority(UART_DMA_RX_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 0);
  HAL_NVIC_EnableIRQ(UART_DMA_RX_IRQn);
}

/**
  * @brief UART MSP De-Initialization
  *        This function frees the hardware resources used in this example:
  *          - Disable the Peripheral's clock
  *          - Revert GPIO, DMA and NVIC configuration to their default state
  * @param huart: UART handle pointer
  * @retval None
  */
void HAL_UART_MspDeInit(UART_HandleTypeDef *huart)
{
  /*##-1- Reset peripherals ##################################################*/
  UART_FORCE_RESET();
  UART_RELEASE_RESET();

  /*##-2- Disable peripherals and GPIO Clocks #################################*/
  /* Configure UART Tx as alternate function  */
  HAL_GPIO_DeInit(UART_TX_GPIO_PORT, UART_TX_PIN);
  /* Configure UART Rx as alternate function  */
  HAL_GPIO_DeInit(UART_RX_GPIO_PORT, UART_RX_PIN);

  /*##-3- Disable the DMA channels ############################################*/
  /* De-Initialize the DMA channel associated to reception process */
  if(huart->hdmarx != 0)
  {
    HAL_DMA_DeInit(huart->hdmarx);
  }
  /* De-Initialize the DMA channel associated to transmission process */
  if(huart->hdmatx != 0)
  {
    HAL_DMA_DeInit(huart->hdmatx);
  }

  /*##-4- Disable the NVIC for DMA ###########################################*/
  HAL_NVIC_DisableIRQ(UART_DMA_TX_IRQn);
  HAL_NVIC_DisableIRQ(UART_DMA_RX_IRQn);
}

/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */

/*****END OF FILE****/
