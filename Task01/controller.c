#include "controller.h"
#include "stdio.h"

static int32_t I = 0;
int32_t u_max = 2000;	// Max input 100% duty cycle

/* Calculate the current velocity in rpm, based on encoder value and time */
int32_t Controller_CalculateVelocity(int16_t encoder, uint32_t time)
 {
	static int16_t encoder_old = 0;
	static uint32_t time_old = 0;
	int16_t diff_encoder = encoder - encoder_old;
	int32_t diff_time = time - time_old;

	// Quadrature encoder
	// PPR = 512 * 4 = 2048
	int16_t speed = ((diff_encoder)*60000) / (2048*(diff_time));

	// Update values
	encoder_old = encoder;
	time_old = time;

	/*	adding negative sign so when facing the motor
	 *	positive speed --> clockwise rotation
	 *	negative speed --> counter-clockwise rotation   */
	return (-speed); // rpm

}

/* Apply a PI-control law to calcuate the control signal for the motor*/
int32_t Controller_PIController(int32_t ref, int32_t vel, uint32_t time)
{
	// PI Control
	int16_t K_p = 100;			// P control
	int16_t K_i = 3;				// I control
	int16_t K_ant = 500;		// Back calculation gain
	static uint32_t time_old = 0;

	int32_t dt = time - time_old;
	int32_t error = ref-vel;

	I += error*dt;

	int32_t control = (K_p*error + K_i*I)/1000;
	int32_t control_sat = control;

	// Saturation
	if(control_sat > u_max)
	{
		control_sat = u_max;
	}
	else if (control_sat < -u_max)
	{
		control_sat = -u_max;
	}

	// Anti-windup Back Calculation
	I += (control_sat - control)*K_ant;

	// Update time
	time_old = time;

	return control_sat;

}

/* Reset internal state variables, such as the integrator */
void Controller_Reset(void)
{
	// Implement saturation & back-calculation for Anti-Windup instead (Line 47-58)
	return;
}
