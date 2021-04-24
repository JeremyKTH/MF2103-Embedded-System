#include "main.h"
#include "application.h"
#include "controller.h"
#include "peripherals.h"
#include "cmsis_os.h"
#include "socket.h"
#include "wizchip_conf.h"
#include <stdio.h>

#define PERIOD_CTRL 10
#define APP_SOCK 0
#define SERVER_PORT 2103

/*----------------------------------------------------------------------------
  Global Variables
 *---------------------------------------------------------------------------*/
// Controller
uint16_t encoder;
uint32_t millisec;
int32_t reference, control, velocity;

// Client Socket
uint8_t server_addr[4] = {192, 168, 0, 10};
int8_t retval_client;
uint8_t sock_status_client;
//recv_buf_len;
uint8_t flag;
/*----------------------------------------------------------------------------
  Functions Declarations
 *---------------------------------------------------------------------------*/
void thread1 (void const *argument);
void thread2 (void const *argument);
void callback(void const *param);

/*----------------------------------------------------------------------------
  Virtual Timer - Declarations
 *---------------------------------------------------------------------------*/
osTimerDef(timer0_handle, callback);
osTimerId timer0; //Timer for control period

/*----------------------------------------------------------------------------
  Thread - Declarations
 *---------------------------------------------------------------------------*/
osThreadId T_ID1; //Velocity calc thread
osThreadId T_ID2; //Send/Receive thread
osThreadId main_ID; //Application Loop thread

osThreadDef(thread1, osPriorityNormal, 1, 0); //Velocity calc thread def
osThreadDef(thread2, osPriorityNormal, 1, 0); //Send/Receive thread def

/*----------------------------------------------------------------------------
  Functions Definitions
	- Application_Setup(): Hardware Initialisation
	- Callback(): 				 Virtual Timer callbacks
 *---------------------------------------------------------------------------*/
int Application_Setup()
{
	encoder = 0;
	velocity = 0;
	control = 0;
	millisec = 0;

	// Enable Motor
	Peripheral_GPIO_EnableMotor();

	// Create Virtual Timer
	timer0 = osTimerCreate(osTimer(timer0_handle), osTimerPeriodic, (void *)0);

	// Suspend Kernel
	osKernelInitialize ();

	// Create Threads 1
	T_ID1 = osThreadCreate(osThread(thread1), NULL);
	T_ID2 = osThreadCreate(osThread(thread2), NULL);
	main_ID = osThreadGetId();

	// Timer Start:
	osTimerStart(timer0, PERIOD_CTRL);

	// Restart Kernel
	osKernelStart ();


	return 0;
}


void callback(void const *param)
{
	switch((uint32_t) param)
	{
		case 0:
			osSignalSet(T_ID1,0x01); 
		break;
	}
}

/*----------------------------------------------------------------------------
	Thread 1 - Get encoder, time and calc velocity
 *---------------------------------------------------------------------------*/
void thread1 (void const *argument)
{
	for(;;) {
		osSignalWait(0x01, osWaitForever);
		
		//system clock [ms]
		millisec = SysTick_ms();
		
		// Get encoder signal
		encoder = Peripheral_Timer_ReadEncoder();
		
		// Get current velocity
		velocity = Controller_CalculateVelocity(encoder, millisec);

		osSignalSet(T_ID2, 0x01);
		osSignalWait(0x01, osWaitForever);

		if (flag == 1)
		{
			Peripheral_PWM_ActuateMotor(control);
		}
		if (flag == 0)
		{
//			Peripheral_PWM_ActuateMotor(0);
			TIM3->CCR1 = 0;
			TIM3->CCR2 = 0;
			control = 0;
			osSignalSet(main_ID, 0x01);
		}
	}
}

/*----------------------------------------------------------------------------
	Thread 2 - Send and Recieve Signals
 *---------------------------------------------------------------------------*/
void thread2 (void const *argument)
{
	for(;;) {
		osSignalWait(0x01, osWaitForever);
		flag = 0;

		if((retval_client = send(APP_SOCK, (uint8_t*)&velocity, sizeof(velocity))) == sizeof(velocity)) //sizeof(velocity) = 4 DATA_LEN
		{
//			printf("Send velocity: %d \n\r", velocity);

			if((retval_client = recv(APP_SOCK, (uint8_t*)&control, sizeof(control))) == sizeof(control))
			{
//				printf("Receive control: %d\n\r", control);
				flag = 1;
			}
			else
			{
//				printf("Failed to receive control!!! \n\r");
			}
		}
		else
		{
//			printf("Failed to send velocity!!! \n\r");
		}

		osSignalSet(T_ID1, 0x01);
	}
}

/*----------------------------------------------------------------------------
	Main Loop
 *---------------------------------------------------------------------------*/
void Application_Loop()
{
	for(;;)
	{
//		printf("Opening socket... ");
		// Open socket
		if((retval_client = socket(APP_SOCK, SOCK_STREAM, SERVER_PORT, SF_TCP_NODELAY)) == APP_SOCK)
		{
//			printf("Success!\n\r");

			// Try to connect to server
//			printf("Connecting to server... ");
			if((retval_client = connect(APP_SOCK, server_addr, SERVER_PORT)) == SOCK_OK)
			{
//				printf("Socket Connected!\n\r");
				retval_client = getsockopt(APP_SOCK, SO_STATUS, &sock_status_client);
				while(sock_status_client == SOCK_ESTABLISHED)
				{
					osSignalWait(0x01, osWaitForever);
					retval_client = getsockopt(APP_SOCK, SO_STATUS, &sock_status_client);
				}
//				printf("Disconnected! \n\r");
			}
			else
			{
//				printf("Failed to connect \n\r");
			}
			close(APP_SOCK);
//			printf("Socket Closed. \n\r");
		}
		else
		{
//			printf("Failed to opend the socket \n\r");
		}
		osDelay(100);
	}
}
