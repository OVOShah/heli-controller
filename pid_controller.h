#ifndef __PID_CONTROLLER_H__
#define __PID_CONTROLLER_H__

// Calculate motor output to stabilize along Z-axis
float LiftPID(float fZAccel);

float RollPID(float fRoll);
float PitchPID(float fPitch);
float YawPID(float fYaw);


#endif __PID_CONTROLLER_H__