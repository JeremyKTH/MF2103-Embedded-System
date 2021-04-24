#include "main.h"

#include "application.h"
#include "controller.h"
#include "peripherals.h"
#include "cmsis_os.h"
#include "socket.h"

#define PERIOD_REF 4000
#define APP_SOCK 0
#define SERVER_PORT 2103

/*----------------------------------------------------------------------------
  Global Variables
 *---------------------------------------------------------------------------*/
int32_t reference, velocity, control;
uint32_t millisec;

uint8_t retval_server;
uint8_t sock_status_server;

/*----------------------------------------------------------------------------
  Functions Declarations
 *---------------------------------------------------------------------------*/
void thread1(void const *arg);
void thread2(void const *arg);
void callback(void const *param);

/*----------------------------------------------------------------------------
 Thread - Declarations
*---------------------------------------------------------------------------*/
osThreadId main_ID;
osThreadId T_ID1;
osThreadId T_ID2;

osThreadDef(thread1, osPriorityNormal, 1, 0);
osThreadDef(thread2, osPriorityNormal, 1, 0);

/*----------------------------------------------------------------------------
  Timer - Declarations
 *---------------------------------------------------------------------------*/
osTimerDef(timer0_handle, callback);
osTimerId timer0;

 /*----------------------------------------------------------------------------
  Functions Definitions
	- Application_Setup(): Hardware Initialisation
 *---------------------------------------------------------------------------*/
int Application_Setup()
{
	// Reset global variables
	reference = 2000;
	velocity = 0;
	control = 0;
	millisec = 0;
	
	// Suspend Kernel
	osKernelInitialize();			
	
	// Create timer
	timer0 = osTimerCreate(osTimer(timer0_handle), osTimerPeriodic, (void *)0);

	//Start the timer
	osTimerStart(timer0, PERIOD_REF);
	
	// Create thread 1 & 2
	T_ID1 = osThreadCreate(osThread(thread1),NULL);
	T_ID2 = osThreadCreate(osThread(thread2),NULL);
	main_ID = osThreadGetId();

	// Restart Kernel
	osKernelStart();
	
	return 0;
}

/*----------------------------------------------------------------------------
	Virtual timer callback functions
 *---------------------------------------------------------------------------*/
void callback(void const *param){
	switch((uint32_t) param){
		case 0:
			osSignalSet(T_ID2, 0x01);
		break;
	}
}

/*----------------------------------------------------------------------------
		Thread 1 - Receive: Velocity
						 - Send: 		Control Signal
 *---------------------------------------------------------------------------*/
void thread1 (void const *arg) {
	for(;;) {
		
		osSignalWait(0x01, osWaitForever);
		
		/*----------------------------------------------------------------------------
		  Receving velocity

			* getsockopt(__, SO_RECVBUF, recv_buf_len)
			  returns the number of bytes received in the received buffer

  		* recv(socket number, pointer buffer, Max length of data in buffer)
				do 3 things:
					1. fetch the data from received buffer and save it into the pointer buffer (velocity)
					2. empties the received buffer
					3. return the actual number of bytes received
		 *---------------------------------------------------------------------------*/
		if ((retval_server = recv(APP_SOCK, (uint8_t*)&velocity, sizeof(velocity))) == sizeof(velocity))
		{
			printf("Received: %d\n\r", velocity);
			millisec = SysTick_ms();
			
		/*----------------------------------------------------------------------------
			Calculate control signal
		 *---------------------------------------------------------------------------*/
			control = Controller_PIController(reference, velocity, millisec);
			
		/*----------------------------------------------------------------------------
			Send control signal
		 *---------------------------------------------------------------------------*/
			if((retval_server = send(APP_SOCK, (uint8_t*)&control, sizeof(control))) == sizeof(control))
			{
				printf("Sent: %d\n\r", control);
			}
			else
			{
				printf("Could not send control signal!\n\r");
				Controller_Reset();
				velocity = 0;
			}
		}
		else
		{
			printf("Could not receive velocity!\n\r");
			Controller_Reset();
			velocity = 0;
		}
		osSignalSet(main_ID,0x01);
	}
}

/*----------------------------------------------------------------------------
		Thread 2 - Toggle reference signal
 *---------------------------------------------------------------------------*/
void thread2(void const *arg) { 																				
	for(;;) {
		osSignalWait(0x01,osWaitForever);
		reference = -reference;	
		Controller_Reset();
	}
}

/*----------------------------------------------------------------------------
		Main Thread - Create socket connections
 *---------------------------------------------------------------------------*/
void Application_Loop() {
	for(;;)
	{
		printf("Opening socket... ");
		if((retval_server = socket(APP_SOCK, SOCK_STREAM, SERVER_PORT, SF_TCP_NODELAY)) == APP_SOCK)
		{
			printf("Success!\n\r");
			//Try to listen to server
			printf("Listening... ");
			if ((retval_server = listen(APP_SOCK)) == SOCK_OK)
			{
				printf("Success!\n\r");
				retval_server = getsockopt(APP_SOCK, SO_STATUS, &sock_status_server);
				while (sock_status_server == SOCK_LISTEN || sock_status_server == SOCK_ESTABLISHED)
				{
					if (sock_status_server == SOCK_ESTABLISHED)
					{
						osSignalSet(T_ID1,0x01);
						osSignalWait(0x01,osWaitForever);
					}
					else
					{
						printf("Something went wrong!\n\r");
						osDelay(10);
					}
					retval_server = getsockopt(APP_SOCK, SO_STATUS, &sock_status_server);
				}
				printf("Disconnected! \n\r");
				Controller_Reset();
				close(APP_SOCK);
				printf("Socket closed!\n\r");
			}
		}
		else // Socket cant open
		{
			printf("Failed to open socket!\n\r");
			
		}
		//Wait 500ms before trying again
		osDelay(500);
	}
}


