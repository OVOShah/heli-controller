#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#include "Sonar.h"
#include "Time.h"

#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"

#define SYSCTL_PERIPH_TRIG          SYSCTL_PERIPH_GPIOE
#define GPIO_SONAR_TRIG_BASE        GPIO_PORTE_BASE
#define GPIO_SONAR_TRIG             GPIO_PIN_0

#define SYSCTL_PERIPH_ECHO          SYSCTL_PERIPH_GPIOC
#define GPIO_SONAR_ECHO_BASE        GPIO_PORTC_BASE
#define GPIO_SONAR_ECHO             GPIO_PIN_5

// Timer used to measure the width of the sonar echo pulse
#define SYSCTL_PERIPH_SONAR_TIMER   SYSCTL_PERIPH_WTIMER0
#define GPIO_SONAR_ALTERNATE        GPIO_PC5_WT0CCP1
#define SONAR_TIMER_BASE            WTIMER0_BASE
#define SONAR_TIMER_INT             INT_WTIMER0B
#define SONAR_TIMER                 TIMER_B
#define SONAR_CAP_EVENT             TIMER_CAPB_EVENT
#define SONAR_TIMER_CFG             TIMER_CFG_B_CAP_TIME_UP

// Implemented based on: http://che126.che.caltech.edu/28015-PING-Sensor-Product-Guide-v2.0.pdf

static volatile uint32_t sonarDistanceDeciUs;
static volatile bool newSonarDistance;

static void SonarHandler(void) {
    static uint32_t prev = 0;
    static bool last_edge = false;

    TimerIntClear(SONAR_TIMER_BASE, SONAR_CAP_EVENT); // Clear interrupt
    uint32_t curr = TimerValueGet(SONAR_TIMER_BASE, SONAR_TIMER); // Read capture value
    bool edge = GPIOPinRead(GPIO_SONAR_ECHO_BASE, GPIO_SONAR_ECHO); // Read the GPIO pin

    if (last_edge && !edge) { // Check that we are going from a positive to falling edge
        if (curr > prev) {
            uint32_t diff = curr - prev; // Calculate diff
            sonarDistanceDeciUs = 10000000UL / (SysCtlClockGet() / diff); // Convert to deci-us
            //UARTprintf("%u %d %d\n", diff, sonarDistanceDeciUs, sonarDistanceDeciUs / 57);
            newSonarDistance = true;
        }
    }

    prev = curr; // Store previous value
    last_edge = edge; // Store last edge
}

bool triggerSonar(void) {
    static uint32_t lastTrigger = 0;

    uint32_t now = millis();
    if ((int32_t)(now - lastTrigger) > 25) { // Trigger every 25ms
        lastTrigger = now;
        GPIOPinWrite(GPIO_SONAR_TRIG_BASE, GPIO_SONAR_TRIG, GPIO_SONAR_TRIG); // Set pin high
        delayMicroseconds(10); // Other sources wait 10us
        GPIOPinWrite(GPIO_SONAR_TRIG_BASE, GPIO_SONAR_TRIG, 0); // Set pin low
        //UARTprintf("%d\n", getSonarDistance());
    }
    return newSonarDistance; // Returns true if a measurement is ready
}

// Returns the distance in mm. Range is 0-3000 mm or -1 if the value is invalid.
int16_t getSonarDistance(angle_t *angle) {
    static const uint8_t US_ROUNDTRIP_CM = 58; // Microseconds (uS) it takes sound to travel round-trip 1cm (2cm total). Calculated at room temperature
    newSonarDistance = false; // Set variable back to false

     if (sonarDistanceDeciUs < 1150 || sonarDistanceDeciUs > 185000) // Datasheet says min is 115us and max is 18.5ms
         return -1;

    // if (fmaxf(fabsf(angle->axis.roll), fabsf(angle->axis.pitch)) > cfg.maxAngleInclinationDistSensor) // Return -1 if it is tilted more than the maximum tilt angle
    //     return -1;

    int16_t distance = sonarDistanceDeciUs / US_ROUNDTRIP_CM; // The output will actually be in mm, as it is in deci-us
    // distance *= cosf(angle->axis.roll * DEG_TO_RAD) * cosf(angle->axis.pitch * DEG_TO_RAD); // Calculate adjacent side

    return distance;
}

void initSonar(void) {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_SONAR_TIMER); // Enable Timer peripheral
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ECHO); // Enable GPIO peripheral
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TRIG); // Enable GPIO peripheral
    SysCtlDelay(2); // Insert a few cycles after enabling the peripheral to allow the clock to be fully activated

    GPIOPinConfigure(GPIO_SONAR_ALTERNATE); // Use alternate function
    GPIOPinTypeTimer(GPIO_SONAR_ECHO_BASE, GPIO_SONAR_ECHO); // Use pin with timer peripheral

    // Split timers and enable timer event up-count timer
    TimerConfigure(SONAR_TIMER_BASE, TIMER_CFG_SPLIT_PAIR | SONAR_TIMER_CFG);

    // Configure the Timer
    TimerControlEvent(SONAR_TIMER_BASE, SONAR_TIMER, TIMER_EVENT_BOTH_EDGES); // Interrupt on both edges
    TimerIntRegister(SONAR_TIMER_BASE, SONAR_TIMER, SonarHandler); // Register interrupt handler
    TimerIntEnable(SONAR_TIMER_BASE, SONAR_CAP_EVENT); // Enable timer capture event interrupt
    IntPrioritySet(SONAR_TIMER_INT, 0); // Configure Timer interrupt priority as 0
    IntEnable(SONAR_TIMER_INT); // Enable Timer interrupt

    GPIOPinTypeGPIOOutput(GPIO_SONAR_TRIG_BASE, GPIO_SONAR_TRIG); // Set pin as output
    GPIOPinWrite(GPIO_SONAR_TRIG_BASE, GPIO_SONAR_TRIG, 0); // Set pin low

    TimerEnable(SONAR_TIMER_BASE, SONAR_TIMER); // Enable Timer
}
