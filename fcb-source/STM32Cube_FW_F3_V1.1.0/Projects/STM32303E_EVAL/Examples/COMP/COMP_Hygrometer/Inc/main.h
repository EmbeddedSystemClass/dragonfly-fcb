/**
  ******************************************************************************
  * @file    COMP/COMP_Hygrometer/Inc/main.h 
  * @author  MCD Application Team
  * @version V1.1.0
  * @date    12-Sept-2014
  * @brief   Header for main.c module
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
  
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f3xx_hal.h"
#include "stm32303e_eval.h"
#include "stm32303e_eval_lcd.h"
#include <math.h>
#include <stdio.h>

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* User can use this section to tailor COMPx instance used and associated 
   resources */
/* Definition for Humidity sensor input/output Channel Pin */
#define HUM_IN_PIN                      GPIO_PIN_0
#define HUM_IN_GPIO_PORT                GPIOB
#define HUM_IN_GPIO_CLK_ENABLE()        __GPIOB_CLK_ENABLE()
#define HUM_IN_GPIO_CLK_DISABLE()       __GPIOB_CLK_DISABLE()

#define HUM_OUT_PIN                     GPIO_PIN_8
#define HUM_OUT_GPIO_PORT               GPIOC
#define HUM_OUT_GPIO_CLK_ENABLE()       __GPIOC_CLK_ENABLE()
#define HUM_OUT_GPIO_CLK_DISABLE()      __GPIOC_CLK_DISABLE()
#define HUM_OUT_AF                      GPIO_AF2_TIM3

/* Definition for COMPx */
#define COMPx                           COMP4
#define COMP_INVERTINGINPUT_DACx        COMP_INVERTINGINPUT_DAC1

/* Definition for DACx clock resources and channel */
#define DACx                            DAC1
#define DACx_CHANNEL                    DAC1_CHANNEL_1
#define __DACx_CLK_ENABLE()             __DAC1_CLK_ENABLE()
#define __DACx_CLK_DISABLE()            __DAC1_CLK_DISABLE()
#define __DACx_FORCE_RESET()            __DAC1_FORCE_RESET()
#define __DACx_RELEASE_RESET()          __DAC1_RELEASE_RESET()

/* Polynomial Expression Coefficients: Capacitance = f(Humidity) */
#define P3 0.00000003903f
#define P2 -0.000008294f
#define P1 0.002188f
#define P0 0.898f

/* Reversed Polynomial Expression Coefficients: Humidity = f(Capacitance)
   RH (%) = -3.4656*10^3 * X^3 + 1.0732*10^4 * X^2 - 1.0457*10^4*X + 3.2459*10^3
   with X = C (read) / C@55%RH */
#define RP3 -3465.6f
#define RP2 10732
#define RP1 -10457
#define RP0 3245.9f

/* Reference Capacitance Value is 180 pF 1%. This capacitance is used in calibration phase */
#define REFCAP 180e-12

/* Resistance Value. The reference capacitance and humidity sensor are 
   charged/discharged through the resistance R159 available on STM32303E-EVAL RevC.
   It's value is 82.5K 1%
*/
#define RES 82500

/* TIM3 PWM frequency value in Hertz. TIM3 is usd to charge/discharge the humidity sensor.
   Min TIM3 PWM frequency is SystemCoreClock/65535 and max TIM3 PWM frequency  = 1/(RES * CAP)
   with RES is the resistance and CAP is the minimum sensor capacitance.
   For HS1101, CAP is equal to 160 pF.
   when SystemCoreClock is 72 MHz, the min frequency is 1099Hz and the max frequency is 75.7 KHz
*/
#define FREQ 1100

/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
