#include "main.h"
#include "application.h"
#include "controller.h"
#include "peripherals.h"
#include "cmsis_os.h"

#define PERIOD_CTRL 10
#define PERIOD_REF 4000


/*----------------------------------------------------------------------------
  Global Variables
 *---------------------------------------------------------------------------*/
uint16_t encoder;
uint32_t millisec;
int32_t reference, velocity, control;


/*----------------------------------------------------------------------------
  Functions Declarations
 *---------------------------------------------------------------------------*/
int Application_Setup();
void thread1 (void const *argument);
void thread2 (void const *argument);
void callback(void const *param);
 
 
/*----------------------------------------------------------------------------
  Virtual Timer - Declarations
 *---------------------------------------------------------------------------*/
osTimerDef(timer0_handle, callback);
osTimerDef(timer1_handle, callback);


/*----------------------------------------------------------------------------
  Thread - Declarations
 *---------------------------------------------------------------------------*/
osThreadId T_ID1;
osThreadId T_ID2;

osThreadDef(thread1, osPriorityAboveNormal, 1, 0);
osThreadDef(thread2, osPriorityNormal, 1, 0);


/*----------------------------------------------------------------------------
  Functions Definitions
	- Application_Setup(): Hardware Initialisation
	- Callback(): 				 Virtual Timer callbacks
 *---------------------------------------------------------------------------*/
int Application_Setup()
{
	reference = 2000;
	velocity = 0;
	control = 0;
	millisec = 0;

	Peripheral_GPIO_EnableMotor();

	return 0;
}


void callback(void const *param)
{
	switch( (uint32_t) param)
	{
		case 0:
			osSignalSet	(T_ID1,0x03);
		break;

		case 1:
			osSignalSet	(T_ID2,0x05);
		break;
	}
}

/*----------------------------------------------------------------------------
	Thread 1 - Encoder, Velocity, Control - Normal Priority
 *---------------------------------------------------------------------------*/
void thread1 (void const *argument)
{
	for(;;){
		
		osSignalWait(0x03,osWaitForever);
		
		//system clock [ms]
		millisec = SysTick_ms();
		
		// Get encoder signal
		encoder = Peripheral_Timer_ReadEncoder();
		
		// Get current velocity
		velocity = Controller_CalculateVelocity(encoder, millisec);
				
		// Calculate control signal
		control = Controller_PIController(reference, velocity, millisec);
	
		// Apply control signal to motor
		Peripheral_PWM_ActuateMotor(control);
		
		// osDelay(PERIOD_CTRL);
		
	}
}

/*----------------------------------------------------------------------------
	Thread 2 - Reference - Above Normal Priority
 *---------------------------------------------------------------------------*/
void thread2 (void const *argument)
{
	for(;;){
				
		osSignalWait(0x05,osWaitForever);
		
		reference = -reference;
		
		// osDelay(PERIOD_REF);
	}
}

/*----------------------------------------------------------------------------
	Main Loop
 *---------------------------------------------------------------------------*/
void Application_Loop()
{
	
	// Create Virtual Timer
	osTimerId timer0 = osTimerCreate(osTimer(timer0_handle), osTimerPeriodic, (void *)0);	
	osTimerId timer1 = osTimerCreate(osTimer(timer1_handle), osTimerPeriodic, (void *)1);	
	
	// Suspend Kernel
	osKernelInitialize ();
	
	osTimerStart(timer0, PERIOD_CTRL);	// Thread 1
	osTimerStart(timer1, PERIOD_REF);		// Thread 2

	// Create Threads 1 & 2
	T_ID1 = osThreadCreate(osThread(thread1), NULL);
	T_ID2 = osThreadCreate(osThread(thread2), NULL);
	
	// Restart Kernel
	osKernelStart ();
	
	osSignalWait(0x01, osWaitForever); 
}