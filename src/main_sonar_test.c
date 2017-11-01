#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_memmap.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "pid_controller.h"
#include "Sonar.h"
#include "UART.h"

int main(void)
{
     SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

     initUART();
     initSonar();
     angle_t tmp;

    while(1)
    {
        if (triggerSonar()){
            int16_t distance = getSonarDistance(tmp);
            UARTprintf("%d", distance);
        }
    }
}