#include "main.h"
#include "stdio.h"
#include "application.h"
#include "controller.h"
#include "peripherals.h"

#define PERIOD_CTRL 10
#define PERIOD_REF 4000

/* Global variables ----------------------------------------------------------*/
int16_t encoder;
int32_t reference, velocity, control;
uint32_t millisec;
uint32_t CCR1, CCR2;

/* Functions -----------------------------------------------------------------*/

/* Run setup needed for all periodic tasks */
int Application_Setup()
{
	// Reset global variables
	reference = 2000;
	velocity = 0;
	control = 0;
	millisec = 0;

	// Initialise hardware
	Peripheral_GPIO_EnableMotor();

	return 0;
}

/* Define what to do in the infinite loop */
void Application_Loop()
{
	// Get time
		millisec = SysTick_ms();

		// Every 10 msec ... sampling rate 100Hz
		if (millisec % PERIOD_CTRL == 0)
		{
			// Read encoder signal
			encoder = Peripheral_Timer_ReadEncoder();

			// Get current velocity
			velocity = Controller_CalculateVelocity(encoder, millisec);

			// Calculate control signal
			control = Controller_PIController(reference, velocity, millisec);

			// Apply control signal to motor
			Peripheral_PWM_ActuateMotor(control);

			// Check value
			CCR1 = TIM3->CCR1;
			CCR2 = TIM3->CCR2;
			printf("CCR1 = %d\t CCR2 =%d\t Control value = %d\n", CCR1, CCR2, control);

			// Every 4 sec ...
			if (millisec % PERIOD_REF == 0)
			{
				// Flip the direction of the reference
				reference = -reference;

			}

			// Prevent from re-entering again
			while(SysTick_ms() % PERIOD_CTRL == 0);
		}
}
