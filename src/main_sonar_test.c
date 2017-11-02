#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include "inc/hw_memmap.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "pid_controller.h"
#include "Sonar.h"
#include "UART.h"
#include "Time.h"
#include "pid_controller.h"

#define M_PIf   3.14159265358979323846f

int main(void)
{
     SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

     initUART();
     initSonar();
     initTime();
     setHeight(300);
     angle_t tmp;
     float x = 0;

     srand(millis());

     int32_t origWave;
     int32_t disturbedWave;
     float pid;
     uint32_t prevTime = 0;

    while(1)
    {
//        if (triggerSonar()){
//            int16_t distance = getSonarDistance(&tmp);
//            UARTprintf("%d\n", distance);
//        }

        x = 0;

        while (x < 2.0*M_PIf){
            origWave = (int32_t)(sin(x)*100 + 300);
            disturbedWave = origWave + (rand()%50 - 25);
            pid = liftPID((float)disturbedWave, (float)(millis()-prevTime));

            UARTprintf("%d %d\n", disturbedWave, pid);

            x += 0.05;
            SysCtlDelay(SysCtlClockGet()/12);
        }

    }
}
