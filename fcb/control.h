#define L 0.30				// Quadcopter arm length
#define At (int) 4187385	// T = A*t_out^2+C
#define Ct (-4.187385)		// (Thrust coeffs)
#define Bq (int) 128305		// Q = B*t_out^2+D
#define Dq (-0.128305)		// (drag torque coeffs, d not used?)

#include "stm32f30x.h"
#include "stm32f3_discovery.h"
#include "stm32f30x_it.h"
#include "stm32f30x_tim.h"
#include "stm32f30x_gpio.h"
#include "stm32f30x_rcc.h"
#include "stm32f30x_misc.h"

uint16_t GetPWM_CCR(float dutycycle);
void TIM3_IRQHandler(void);
void TIM3_Setup(void);
void TIM3_SetupIRQ(void);
void ControlAllocation(void);
void SetControlSignals(void);