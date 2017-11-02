#include <stdint.h>
#include <stdbool.h>
#include "pid_controller.h"

float error, prevError;

float Kp = 0.5;
float Ki = 0.1;
float Kd = 10;
float stableHeight = 100;

bool setHeight(float height){
	if (height > 1000 || height < 10) return false;
	stableHeight = height;
	return true;
}

float liftPID(float curHeight, float dt){
	error = stableHeight - curHeight;

	// P-Term
	float pTerm = Kp * error;

	// I-Term
	static float iTerm = 0;
	iTerm += Ki * error;

	// D-Term
	float dTerm = Kd * (error - prevError) / dt;
	//TODO: smoothing on derivative??
	
	prevError = error;

	//TODO: Limit PID to maximum motor speed
	return pTerm + iTerm + dTerm;
}
