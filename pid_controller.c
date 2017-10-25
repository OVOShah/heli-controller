#include <stdint.h>
#include <stdbool.h>

float error, previous_error;
float lastZAccel = 0;

float Kp = 0;
float Kd = 10;

// Start with PD controller then tune + add integral 
float LiftPID(float fZAccel){
	error = -fZAccel;
	float pTerm = Kp * error;
	float dTerm = Kd * (fZAccel - lastZAccel);
	
	lastZAccel = fZAccel;

	float PIDValue = pTerm - dTerm;

	//TODO: Limit PID to maximum motor speed

	return PIDValue;
}