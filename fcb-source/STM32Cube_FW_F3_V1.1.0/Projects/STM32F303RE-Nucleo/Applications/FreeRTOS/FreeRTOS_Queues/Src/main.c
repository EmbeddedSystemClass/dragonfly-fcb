/**
  ******************************************************************************
  * @file    FreeRTOS/FreeRTOS_Queues/Src/main.c
  * @author  MCD Application Team
  * @version V1.1.0
  * @date    12-Sept-2014
  * @brief   Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2014 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
#define blckqSTACK_SIZE   configMINIMAL_STACK_SIZE
#define QUEUE_SIZE        (uint32_t) 1

/* Private variables ---------------------------------------------------------*/
osMessageQId osQueue;
uint32_t ProducerValue = 0, ConsumerValue = 0;

/* Private function prototypes -----------------------------------------------*/

/* Thread function that creates an incrementing number and posts it on a queue */
static void MessageQueueProducer(const void *argument);

/* Thread function that removes the incrementing number from a queue and checks that
   it is the expected number */
static void MessageQueueConsumer(const void *argument);

void SystemClock_Config(void);
static void Error_Handler(void);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{
  /* STM32F3xx HAL library initialization:
       - Configure the Flash prefetch
       - Configure the Systick to generate an interrupt each 1 msec
       - Set NVIC Group Priority to 4
       - Low Level Initialization
     */
  HAL_Init();

  /* Initialize LED */
  BSP_LED_Init(LED2);

  /* Configure the System clock to 64 MHz */
  SystemClock_Config();

  /* Create the queue used by the two tasks to pass the incrementing number.
  Pass a pointer to the queue in the parameter structure */
  osMessageQDef(osqueue, QUEUE_SIZE, uint16_t);
  osQueue = osMessageCreate(osMessageQ(osqueue), NULL);

  /* Note that the producer has a lower priority than the consumer when the tasks are
     spawned */
  osThreadDef(QCons, MessageQueueConsumer, osPriorityBelowNormal, 0, blckqSTACK_SIZE);
  osThreadCreate(osThread(QCons), NULL);

  osThreadDef(QProd, MessageQueueProducer, osPriorityBelowNormal, 0, blckqSTACK_SIZE);
  osThreadCreate(osThread(QProd), NULL);

  /* Start scheduler */
  osKernelStart(NULL, NULL);

  /* We should never get here as control is now taken by the scheduler */
  for (;;);

}

/**
  * @brief  Message Queue Producer Thread.
  * @param  argument: Not used
  * @retval None
  */
static void MessageQueueProducer(const void *argument)
{
  for (;;)
  {
    if (osMessagePut(osQueue, ProducerValue, 100) != osOK)
    {
      /* Toggle LED2 to indicate error */
      BSP_LED_Toggle(LED2);
    }
    else
    {
      /* Increment the variable we are going to post next time round.  The
      consumer will expect the numbers to follow in numerical order */
      ++ProducerValue;

      osDelay(1000);
    }
  }
}

/**
  * @brief  Message Queue Consumer Thread.
  * @param  argument: Not used
  * @retval None
  */
static void MessageQueueConsumer(const void *argument)
{
  osEvent event;

  for (;;)
  {
    /* Get the message from the queue */
    event = osMessageGet(osQueue, 100);

    if (event.status == osEventMessage)
    {
      if (event.value.v != ConsumerValue)
      {
        /* Catch-up */
        ConsumerValue = event.value.v;

        /* Toggle LED2 to indicate error */
        BSP_LED_Toggle(LED2);
      }
      else
      {
        /* Increment the value we expect to remove from the queue next time
        round */
        ++ConsumerValue;
      }
    }
  }
}

/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow : 
  *            System Clock source            = PLL (HSI)
  *            SYSCLK(Hz)                     = 64000000
  *            HCLK(Hz)                       = 64000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 2
  *            APB2 Prescaler                 = 1
  *            HSI Frequency(Hz)              = 8000000
  *            PREDIV                         = RCC_PREDIV_DIV2 (2)
  *            PLLMUL                         = RCC_PLL_MUL16 (16)
  *            Flash Latency(WS)              = 2
  * @param  None
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;
  
  /* HSI Oscillator already ON after system reset, activate PLL with HSI as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_NONE;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV2;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct)!= HAL_OK)
  {
    Error_Handler();
  }

  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 
     clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;  
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2)!= HAL_OK)
  {
    Error_Handler();
  }
}
/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
static void Error_Handler(void)
{
  while(1)
  {
    /* Toggle LED2 fast */
    BSP_LED_Toggle(LED2);
    HAL_Delay(40);
  }
}

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *   where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {}
}
#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
