#ifndef __sonar_h__
#define __sonar_h__

#include "types.h"

void initSonar(void);
bool triggerSonar(void);

int16_t getSonarDistance(angle_t *angle);

#endif