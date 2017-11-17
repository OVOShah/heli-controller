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
#include "motor_driver.h"

#define M_PIf   3.14159265358979323846f

int main(void)
{
     SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

     initUART();
     initSonar();
     initTime();
     initPWM();
     setHeight(300);
     angle_t tmp;

     float pid;
     uint32_t prevTime = 0;

    while(1)
    {
//        if (triggerSonar()){
//            int16_t distance = getSonarDistance(&tmp);
//            if (distance > 0) UARTprintf("%d\n", distance);
//        }

          driveMotor(1, 1);
          delay(1000);
          driveMotor(2, 2);
          delay(1000);
          driveMotor(16, 16);
          delay(1000);

    }
}
