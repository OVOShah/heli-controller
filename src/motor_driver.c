/*
 * motor_driver.c
 *
 *  Created on: Nov 16, 2017
 *      Author: Rachit
 */

#include <stdint.h>
#include <stdbool.h>

#include "inc/hw_memmap.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/pwm.h"
#include "driverlib/gpio.h"

void initPWM(void){

    //
    // Set the PWM clock to the system clock.
    //
    ROM_SysCtlPWMClockSet(SYSCTL_PWMDIV_1);

    //
    // Enable the PWM0 peripheral
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM1);

    //
    // For this example PWM0 is used with PortB Pins 6 and 7.  The actual port
    // and pins used may be different on your part, consult the data sheet for
    // more information.  GPIO port B needs to be enabled so these pins can be
    // used.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    //
    // Configure the GPIO pin muxing to select PWM functions for these pins.
    // This step selects which alternate function is available for these pins.
    // This is necessary if your part supports GPIO pin function muxing.
    // Consult the data sheet to see which functions are allocated per pin.
    //
    ROM_GPIOPinConfigure(GPIO_PA6_M1PWM2);
    //GPIOPinConfigure(GPIO_PB7_M0PWM1);

    //
    // Configure the GPIO pad for PWM function on pins PB6 and PB7.  Consult
    // the data sheet to see which functions are allocated per pin.
    //
    ROM_GPIOPinTypePWM(GPIO_PORTA_BASE, GPIO_PIN_6);
    //GPIOPinTypePWM(GPIO_PORTB_BASE, GPIO_PIN_7);

    //
    // Wait for the PWM0 module to be ready.
    //
    while(!ROM_SysCtlPeripheralReady(SYSCTL_PERIPH_PWM1))
    {
    }
    //
    // Configure the PWM generator for count down mode with immediate updates
    // to the parameters.
    //
    ROM_PWMGenConfigure(PWM1_BASE, PWM_GEN_1,
    PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);

    //
    // Set the PWM period to 250Hz.  To calculate the appropriate parameter
    // use the following equation: N = (1 / f) * SysClk.  Where N is the
    // function parameter, f is the desired frequency, and SysClk is the
    // system clock frequency.
    // In this case you get: (1 / 250Hz) * 16MHz = 64000 cycles.  Note that
    // the maximum period you can set is 2^16 - 1.
    // TODO: modify this calculation to use the clock frequency that you are
    // using.
    //
    ROM_PWMGenPeriodSet(PWM1_BASE, PWM_GEN_1, 64000);

    //
    // Start the timers in generator 0.
    //
    ROM_PWMGenEnable(PWM1_BASE, PWM_GEN_1);

    //
    // Enable the outputs.
    //
    ROM_PWMOutputState(PWM1_BASE, (PWM_OUT_0_BIT | PWM_OUT_2_BIT), true);
}

void driveMotor(uint32_t motor1){

    //
    // Set PWM0 PD0 to a duty cycle of 25%.  You set the duty cycle as a
    // function of the period.  Since the period was set above, you can use the
    // PWMGenPeriodGet() function.  For this example the PWM will be high for
    // 25% of the time or 16000 clock cycles (64000 / 4).
    //
    PWMPulseWidthSet(PWM1_BASE, PWM_OUT_2,
                     PWMGenPeriodGet(PWM1_BASE, PWM_GEN_1) / motor1);
}


