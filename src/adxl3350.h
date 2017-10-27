#ifndef __time_h__
#define __time_h__

typedef struct {
	float gX_acceleration, gY_acceleration, gZ_acceleration;
} acceleration_t;

void InitADC(void);
acceleration_t GetAccel(void);

#endif
