#ifndef __PID_CONTROLLER_H__
#define __PID_CONTROLLER_H__

float liftPID(float curHeight, float dt);
bool setHeight(float height);

float rollPID(float curRoll, float dt);
float pitchPID(float curPitch, float dt);
float yawPID(float curYaw, float dt);


#endif
