#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_memmap.h"
#include "driverlib/adc.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "adxl3350.h"

#define ACCEL_DESCR 0.08

//
// This array is used for storing the data read from the ADC FIFO. It
// must be as large as the FIFO for the sequencer in use.  This example
// uses sequence 3 which has a FIFO depth of 1.  If another sequence
// was used with a deeper FIFO, then the array size must be changed.
//
uint32_t pui32ADC0Value[3];

float k_voltage_filt = 0.001;

void InitADC(void)
{
    //
    // The ADC0 peripheral must be enabled for use.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);

    //
    // For this example ADC0 is used with AIN0 on port E7.
    // The actual port and pins used may be different on your part, consult
    // the data sheet for more information.  GPIO port E needs to be enabled
    // so these pins can be used.
    // TODO: change this to whichever GPIO port you are using. This has been changed to port D.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);

    //
    // Select the analog ADC function for these pins.
    // Consult the data sheet to see which functions are allocated per pin.
    // TODO: change this to select the port/pin you are using.
    //
    GPIOPinTypeADC(GPIO_PORTD_BASE, GPIO_PIN_0);
    GPIOPinTypeADC(GPIO_PORTD_BASE, GPIO_PIN_1);
    GPIOPinTypeADC(GPIO_PORTD_BASE, GPIO_PIN_2);

    //
    // Enable sample sequence 3 with a processor signal trigger.  Sequence 3
    // will do a single sample when the processor sends a signal to start the
    // conversion.  Each ADC module has 4 programmable sequences, sequence 0
    // to sequence 3.  This example is arbitrarily using sequence 3.
    //
    ADCSequenceConfigure(ADC0_BASE, 2, ADC_TRIGGER_PROCESSOR, 0);

    //
    // Configure step 0 on sequence 3.  Sample channel 0 (ADC_CTL_CH0) in
    // single-ended mode (default) and configure the interrupt flag
    // (ADC_CTL_IE) to be set when the sample is done.  Tell the ADC logic
    // that this is the last conversion on sequence 3 (ADC_CTL_END).  Sequence
    // 3 has only one programmable step.  Sequence 1 and 2 have 4 steps, and
    // sequence 0 has 8 programmable steps.  Since we are only doing a single
    // conversion using sequence 3 we will only configure step 0.  For more
    // information on the ADC sequences and steps, reference the datasheet.
    //
    ADCSequenceStepConfigure(ADC0_BASE, 2, 0, ADC_CTL_CH7);
    ADCSequenceStepConfigure(ADC0_BASE, 2, 1, ADC_CTL_CH6);
    ADCSequenceStepConfigure(ADC0_BASE, 2, 2, ADC_CTL_CH5 | ADC_CTL_IE | ADC_CTL_END);

    //
    // Since sample sequence 3 is now configured, it must be enabled.
    //
    ADCSequenceEnable(ADC0_BASE, 2);

    //
    // Clear the interrupt status flag.  This is done to make sure the
    // interrupt flag is cleared before we sample.
    //
    ADCIntClear(ADC0_BASE, 2);
}

acceleration_t GetAccel(void){

    static float gX_voltage = 0;
    static float gY_voltage = 0;
    static float gZ_voltage = 0;

    //
    // Trigger the ADC conversion.
    //
    ADCProcessorTrigger(ADC0_BASE, 2);

    //
    // Wait for conversion to be completed.
    //
    while(!ADCIntStatus(ADC0_BASE, 2, false))
    {
    }

    //
    // Clear the ADC interrupt flag.
    //
    ADCIntClear(ADC0_BASE, 2);

    //
    // Read ADC Value.
    //
    ADCSequenceDataGet(ADC0_BASE, 2, pui32ADC0Value);

    float gX_voltage_raw  = (3.3 / 4096.0)*pui32ADC0Value[0];
    gX_voltage += k_voltage_filt*(gX_voltage_raw - gX_voltage);

    float gY_voltage_raw = (3.3 / 4096.0)*pui32ADC0Value[1];
    gY_voltage += k_voltage_filt*(gY_voltage_raw - gY_voltage);

    float gZ_voltage_raw = (3.3 / 4096.0)*pui32ADC0Value[2];
    gZ_voltage += k_voltage_filt*(gZ_voltage_raw - gZ_voltage);

    // Calculate acceleration from voltage
    float gX_acceleration = (gX_voltage -1.5)/0.3;
    float gY_acceleration = (gY_voltage -1.5)/0.3;
    float gZ_acceleration = (gZ_voltage -1.5)/0.3;

    if(gX_acceleration < ACCEL_DESCR)
    {
        if(gX_acceleration > 0){gX_acceleration = 0;}
    }
    if(gX_acceleration < 0){
        if(gX_acceleration > -ACCEL_DESCR){gX_acceleration = 0;}
    }
    if(gY_acceleration < ACCEL_DESCR)
    {
        if(gY_acceleration > 0){gY_acceleration = 0;}
    }
    if(gY_acceleration < 0)
    {
            if(gY_acceleration > -ACCEL_DESCR){gY_acceleration = 0;}
    }
    if(gZ_acceleration < ACCEL_DESCR)
    {
        if(gZ_acceleration > 0){gZ_acceleration = 0;}
    }
    if(gZ_acceleration < 0)
    {
        if(gZ_acceleration > -ACCEL_DESCR){gZ_acceleration = 0;}
    }

    acceleration_t accel;
    accel.gX_acceleration = gX_acceleration;
    accel.gY_acceleration = gY_acceleration;
    accel.gZ_acceleration = gZ_acceleration;
    return accel;
}
