#include "stm32l4xx.h"
#include "peripherals.h"



/* Enable both half-bridges to drive the motor */
void Peripheral_GPIO_EnableMotor(void)
{
	GPIOA->BSRR = GPIO_BSRR_BS5;
	GPIOA->BSRR = GPIO_BSRR_BS6;

	return;
}

/* Drive the motor in both directions */
void Peripheral_PWM_ActuateMotor(int16_t vel)
{
	/*	Channel 1: CW
	 *	Channel 2: CCW  */

	// Clockwise
	if(vel > 0){
		TIM3->CCR1 = vel;
		TIM3->CCR2 = 0;
	}
	// Counter-Clockwise
	if (vel < 0){
		TIM3->CCR1 = 0;
		TIM3->CCR2 = -vel;
	}
	return;
}

/* Read the counter register to get the encoder state */
int16_t Peripheral_Timer_ReadEncoder(void)
{
	return TIM1->CNT;
}
