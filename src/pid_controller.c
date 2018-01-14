#include <stdint.h>
#include <stdbool.h>
#include "pid_controller.h"

float error, prevError;

float Kp = 0.5;
float Ki = 0.01;
float Kd = 10;
float stablePitch = 0;

bool setPitch(float pitch){
	if (pitch > 1000 || pitch < 10) return false;
	stablePitch = pitch;
	return true;
}

float pitchPID(float curPitch, float dt){
	error = stablePitch - curPitch;

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
