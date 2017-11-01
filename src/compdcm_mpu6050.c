//*****************************************************************************
//
// compdcm_mpu6050.c - Example use of the SensorLib with the MPU6050
//
// Copyright (c) 2013-2017 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
// 
// Texas Instruments (TI) is supplying this software for use solely and
// exclusively on TI's microcontroller products. The software is owned by
// TI and/or its suppliers, and is protected under applicable copyright
// laws. You may not combine this software with "viral" open-source
// software in order to form a larger program.
// 
// THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
// NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
// NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
// CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
// DAMAGES, FOR ANY REASON WHATSOEVER.
// 
// This is part of revision 2.1.4.178 of the EK-TM4C123GXL Firmware Package.
//
//*****************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"
#include "sensorlib/hw_mpu6050.h"
#include "sensorlib/hw_ak8975.h"
#include "sensorlib/i2cm_drv.h"
#include "sensorlib/ak8975.h"
#include "sensorlib/mpu6050.h"
#include "sensorlib/comp_dcm.h"
#include "drivers/rgb.h"

#include "UART.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Six Axis Sensor Fusion with the MPU6050 and Complimentary-Filtered
//! DCM (compdcm_mpu6050)</h1>
//!
//! This example demonstrates the basic use of the Sensor Library, TM4C123G
//! LaunchPad and SensHub BoosterPack to obtain nine axis motion measurements
//! from the MPU6050.  The example fuses the nine axis measurements into a set
//! of Euler angles: roll, pitch and yaw.  It also produces the rotation
//! quaternions.  The fusion mechanism demonstrated is complimentary-filtered
//! direct cosine matrix (DCM) algorithm is provided as part of the Sensor
//! Library.
//!
//! Connect a serial terminal program to the LaunchPad's ICDI virtual serial
//! port at 115,200 baud.  Use eight bits per byte, no parity and one stop bit.
//! The raw sensor measurements, Euler angles and quaternions are printed to
//! the terminal.  The RGB LED begins to blink at 1Hz after initialization is
//! completed and the example application is running.
//
//*****************************************************************************

//*****************************************************************************
//
// Define MPU6050 I2C Address.
//
//*****************************************************************************
#define MPU6050_I2C_ADDRESS     0x68

//*****************************************************************************
//
// Global array for holding the color values for the RGB.
//
//*****************************************************************************
uint32_t g_pui32Colors[3];

//*****************************************************************************
//
// Global instance structure for the I2C master driver.
//
//*****************************************************************************
tI2CMInstance g_sI2CInst;

//*****************************************************************************
//
// Global instance structure for the ISL29023 sensor driver.
//
//*****************************************************************************
tMPU6050 g_sMPU6050Inst;

//*****************************************************************************
//
// Global Instance structure to manage the DCM state.
//
//*****************************************************************************
tCompDCM g_sCompDCMInst;

//*****************************************************************************
//
// Global flags to alert main that MPU6050 I2C transaction is complete
//
//*****************************************************************************
volatile uint_fast8_t g_vui8I2CDoneFlag;

//*****************************************************************************
//
// Global flags to alert main that MPU6050 I2C transaction error has occurred.
//
//*****************************************************************************
volatile uint_fast8_t g_vui8ErrorFlag;

//*****************************************************************************
//
// Global flags to alert main that MPU6050 data is ready to be retrieved.
//
//*****************************************************************************
volatile uint_fast8_t g_vui8DataFlag;

//*****************************************************************************
//
// Global counter to control and slow down the rate of data to the terminal.
//
//*****************************************************************************
#define PRINT_SKIP_COUNT        10

uint32_t g_ui32PrintSkipCounter;

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, uint32_t ui32Line)
{
}
#endif

//*****************************************************************************
//
// MPU6050 Sensor callback function.  Called at the end of MPU6050 sensor
// driver transactions. This is called from I2C interrupt context. Therefore,
// we just set a flag and let main do the bulk of the computations and display.
//
//*****************************************************************************
void
MPU6050AppCallback(void *pvCallbackData, uint_fast8_t ui8Status)
{
    //
    // If the transaction succeeded set the data flag to indicate to
    // application that this transaction is complete and data may be ready.
    //
    if(ui8Status == I2CM_STATUS_SUCCESS)
    {
        g_vui8I2CDoneFlag = 1;
    }

    //
    // Store the most recent status in case it was an error condition
    //
    g_vui8ErrorFlag = ui8Status;
}

//*****************************************************************************
//
// Called by the NVIC as a result of GPIO port B interrupt event. For this
// application GPIO port B pin 2 is the interrupt line for the MPU6050
//
//*****************************************************************************
void
IntGPIOb(void)
{
    unsigned long ulStatus;

    ulStatus = GPIOIntStatus(GPIO_PORTB_BASE, true);

    //
    // Clear all the pin interrupts that are set
    //
    GPIOIntClear(GPIO_PORTB_BASE, ulStatus);

    if(ulStatus & GPIO_PIN_2)
    {
        //
        // MPU6050 Data is ready for retrieval and processing.
        //
        MPU6050DataRead(&g_sMPU6050Inst, MPU6050AppCallback, &g_sMPU6050Inst);
    }
}

//*****************************************************************************
//
// Called by the NVIC as a result of I2C3 Interrupt. I2C3 is the I2C connection
// to the MPU6050.
//
//*****************************************************************************
void
MPU6050I2CIntHandler(void)
{
    //
    // Pass through to the I2CM interrupt handler provided by sensor library.
    // This is required to be at application level so that I2CMIntHandler can
    // receive the instance structure pointer as an argument.
    //
    I2CMIntHandler(&g_sI2CInst);
}

//*****************************************************************************
//
// MPU6050 Application error handler. Show the user if we have encountered an
// I2C error.
//
//*****************************************************************************
void
MPU6050AppErrorHandler(char *pcFilename, uint_fast32_t ui32Line)
{
    //
    // Set terminal color to red and print error status and locations
    //
    UARTprintf("\033[31;1m");
    UARTprintf("Error: %d, File: %s, Line: %d\n"
               "See I2C status definitions in sensorlib\\i2cm_drv.h\n",
               g_vui8ErrorFlag, pcFilename, ui32Line);

    //
    // Return terminal color to normal
    //
    UARTprintf("\033[0m");

    //
    // Set RGB Color to RED
    //
    g_pui32Colors[0] = 0xFFFF;
    g_pui32Colors[1] = 0;
    g_pui32Colors[2] = 0;
    RGBColorSet(g_pui32Colors);

    //
    // Increase blink rate to get attention
    //
    RGBBlinkRateSet(10.0f);

    //
    // Go to sleep wait for interventions.  A more robust application could
    // attempt corrective actions here.
    //
    while(1)
    {
        //
        // Do Nothing
        //
    }
}

//*****************************************************************************
//
// Function to wait for the MPU6050 transactions to complete. Use this to spin
// wait on the I2C bus.
//
//*****************************************************************************
void
MPU6050AppI2CWait(char *pcFilename, uint_fast32_t ui32Line)
{
    //
    // Put the processor to sleep while we wait for the I2C driver to
    // indicate that the transaction is complete.
    //
    while((g_vui8I2CDoneFlag == 0) && (g_vui8ErrorFlag == 0))
    {
        //
        // Do Nothing
        //
    }

    //
    // If an error occurred call the error handler immediately.
    //
    if(g_vui8ErrorFlag)
    {
        MPU6050AppErrorHandler(pcFilename, ui32Line);
    }

    //
    // clear the data flag for next use.
    //
    g_vui8I2CDoneFlag = 0;
}

void
InitMPU(void){
    //
    // The I2C3 peripheral must be enabled before use.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C3);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);

    //
    // Configure the pin muxing for I2C3 functions on port D0 and D1.
    //
    ROM_GPIOPinConfigure(GPIO_PD0_I2C3SCL);
    ROM_GPIOPinConfigure(GPIO_PD1_I2C3SDA);

    //
    // Select the I2C function for these pins.  This function will also
    // configure the GPIO pins pins for I2C operation, setting them to
    // open-drain operation with weak pull-ups.  Consult the data sheet
    // to see which functions are allocated per pin.
    //
    GPIOPinTypeI2CSCL(GPIO_PORTD_BASE, GPIO_PIN_0);
    ROM_GPIOPinTypeI2C(GPIO_PORTD_BASE, GPIO_PIN_1);

    //
    // Configure and Enable the GPIO interrupt. Used for INT signal from the
    // MPU6050
    //
    ROM_GPIOPinTypeGPIOInput(GPIO_PORTB_BASE, GPIO_PIN_2);
    GPIOIntEnable(GPIO_PORTB_BASE, GPIO_PIN_2);
    ROM_GPIOIntTypeSet(GPIO_PORTB_BASE, GPIO_PIN_2, GPIO_FALLING_EDGE);
    ROM_IntEnable(INT_GPIOB);

    //
    // Keep only some parts of the systems running while in sleep mode.
    // GPIOB is for the MPU6050 interrupt pin.
    // UART0 is the virtual serial port
    // TIMER0, TIMER1 and WTIMER5 are used by the RGB driver
    // I2C3 is the I2C interface to the ISL29023
    //
    ROM_SysCtlPeripheralClockGating(true);
    ROM_SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_GPIOB);
    ROM_SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_UART0);
    ROM_SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_TIMER0);
    ROM_SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_TIMER1);
    ROM_SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_I2C3);
    ROM_SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_WTIMER5);

    //
    // Enable interrupts to the processor.
    //
    ROM_IntMasterEnable();

    //
    // Initialize I2C3 peripheral.
    //
    I2CMInit(&g_sI2CInst, I2C3_BASE, INT_I2C3, 0xff, 0xff,
             ROM_SysCtlClockGet());

    //
    // Initialize the MPU6050 Driver.
    //
    MPU6050Init(&g_sMPU6050Inst, &g_sI2CInst, MPU6050_I2C_ADDRESS,
                MPU6050AppCallback, &g_sMPU6050Inst);

    //
    // Wait for transaction to complete
    //
    MPU6050AppI2CWait(__FILE__, __LINE__);

    //
    // Write application specifice sensor configuration such as filter settings
    // and sensor range settings.
    //
    g_sMPU6050Inst.pui8Data[0] = MPU6050_CONFIG_DLPF_CFG_94_98;
    g_sMPU6050Inst.pui8Data[1] = MPU6050_GYRO_CONFIG_FS_SEL_250;
    g_sMPU6050Inst.pui8Data[2] = (MPU6050_ACCEL_CONFIG_AFS_SEL_2G);
    MPU6050Write(&g_sMPU6050Inst, MPU6050_O_CONFIG, g_sMPU6050Inst.pui8Data, 3,
                 MPU6050AppCallback, &g_sMPU6050Inst);

    //
    // Wait for transaction to complete
    //
    MPU6050AppI2CWait(__FILE__, __LINE__);

    //
    // Configure the data ready interrupt pin output of the MPU6050.
    //
    g_sMPU6050Inst.pui8Data[0] = MPU6050_INT_PIN_CFG_INT_LEVEL |
                                    MPU6050_INT_PIN_CFG_INT_RD_CLEAR |
                                    MPU6050_INT_PIN_CFG_LATCH_INT_EN;
    g_sMPU6050Inst.pui8Data[1] = MPU6050_INT_ENABLE_DATA_RDY_EN;
    MPU6050Write(&g_sMPU6050Inst, MPU6050_O_INT_PIN_CFG,
                 g_sMPU6050Inst.pui8Data, 2, MPU6050AppCallback,
                 &g_sMPU6050Inst);

    //
    // Wait for transaction to complete
    //
    MPU6050AppI2CWait(__FILE__, __LINE__);

    //
    // Initialize the DCM system. 50 hz sample rate.
    // accel weight = .2, gyro weight = .8, mag weight = .2
    //
    CompDCMInit(&g_sCompDCMInst, 1.0f / 50.0f, 0.2f, 0.6f, 0.2f);
}

//*****************************************************************************
//
// Main application entry point.
//
//*****************************************************************************
int
main(void)
{
    int_fast32_t i32IPart[9], i32FPart[9];
    uint_fast32_t ui32Idx, ui32CompDCMStarted;
    float pfData[9];
    float *pfAccel, *pfGyro, *pfEulers, *pfQuaternion;

    //
    // Initialize convenience pointers that clean up and clarify the code
    // meaning. We want all the data in a single contiguous array so that
    // we can make our pretty printing easier later.
    //
    pfAccel = pfData;
    pfGyro = pfData + 3;
    pfEulers = pfData + 6;

    //
    // Setup the system clock to run at 40 Mhz from PLL with crystal reference
    //
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ |
                       SYSCTL_OSC_MAIN);

    //
    // Enable port B used for motion interrupt.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);

    //
    // Initialize the UART.
    //
    initUART();

    //
    // Print the welcome message to the terminal.
    //
    UARTprintf("\033[2JMPU6050 Raw Example\n");

    //
    // Set the color to a purple approximation.
    //
    g_pui32Colors[RED] = 0x8000;
    g_pui32Colors[BLUE] = 0x8000;
    g_pui32Colors[GREEN] = 0x0000;

    //
    // Initialize RGB driver.
    //
    RGBInit(0);
    RGBColorSet(g_pui32Colors);
    RGBIntensitySet(0.5f);
    RGBEnable();

    InitMPU();

    UARTprintf("\033[2J\033[H");
    UARTprintf("MPU6050 9-Axis Simple Data Application Example\n\n");
    UARTprintf("\033[20GX\033[31G|\033[43GY\033[54G|\033[66GZ\n\n");
    UARTprintf("Accel\033[8G|\033[31G|\033[54G|\n\n");
    UARTprintf("Gyro\033[8G|\033[31G|\033[54G|\n\n");
    UARTprintf("Mag\033[8G|\033[31G|\033[54G|\n\n");
    UARTprintf("\n\033[20GRoll\033[31G|\033[43GPitch\033[54G|\033[66GYaw\n\n");
    UARTprintf("Eulers\033[8G|\033[31G|\033[54G|\n\n");

    UARTprintf("\n\033[17GQ1\033[26G|\033[35GQ2\033[44G|\033[53GQ3\033[62G|"
               "\033[71GQ4\n\n");
    UARTprintf("Q\033[8G|\033[26G|\033[44G|\033[62G|\n\n");

    //
    // Enable blinking indicates config finished successfully
    //
    RGBBlinkRateSet(1.0f);

    ui32CompDCMStarted = 0;

    while(1)
    {
        //
        // Go to sleep mode while waiting for data ready.
        //
        while(!g_vui8I2CDoneFlag)
        {
            ROM_SysCtlSleep();
        }

        //
        // Clear the flag
        //
        g_vui8I2CDoneFlag = 0;

        //
        // Get floating point version of the Accel Data in m/s^2.
        //
        MPU6050DataAccelGetFloat(&g_sMPU6050Inst, pfAccel, pfAccel + 1,
                                 pfAccel + 2);

        //
        // Get floating point version of angular velocities in rad/sec
        //
        MPU6050DataGyroGetFloat(&g_sMPU6050Inst, pfGyro, pfGyro + 1,
                                pfGyro + 2);

        //
        // Check if this is our first data ever.
        //
        if(ui32CompDCMStarted == 0)
        {
            //
            // Set flag indicating that DCM is started.
            // Perform the seeding of the DCM with the first data set.
            //
            ui32CompDCMStarted = 1;
            CompDCMAccelUpdate(&g_sCompDCMInst, pfAccel[0], pfAccel[1],
                               pfAccel[2]);
            CompDCMGyroUpdate(&g_sCompDCMInst, pfGyro[0], pfGyro[1],
                              pfGyro[2]);
            CompDCMStart(&g_sCompDCMInst);
        }
        else
        {
            //
            // DCM Is already started.  Perform the incremental update.
            //
            CompDCMAccelUpdate(&g_sCompDCMInst, pfAccel[0], pfAccel[1],
                               pfAccel[2]);
            CompDCMGyroUpdate(&g_sCompDCMInst, -pfGyro[0], -pfGyro[1],
                              -pfGyro[2]);
            CompDCMUpdate(&g_sCompDCMInst);
        }

        //
        // Increment the skip counter.  Skip counter is used so we do not
        // overflow the UART with data.
        //
        g_ui32PrintSkipCounter++;
        if(g_ui32PrintSkipCounter >= PRINT_SKIP_COUNT)
        {
            //
            // Reset skip counter.
            //
            g_ui32PrintSkipCounter = 0;

            //
            // Get Euler data. (Roll Pitch Yaw)
            //
            CompDCMComputeEulers(&g_sCompDCMInst, pfEulers, pfEulers + 1,
                                 pfEulers + 2);

            //
            // Convert Eulers to degrees. 180/PI = 57.29...
            // Convert Yaw to 0 to 360 to approximate compass headings.
            //
            pfEulers[0] *= 57.295779513082320876798154814105f;
            pfEulers[1] *= 57.295779513082320876798154814105f;
            pfEulers[2] *= 57.295779513082320876798154814105f;
            if(pfEulers[2] < 0)
            {
                pfEulers[2] += 360.0f;
            }

            //
            // Now drop back to using the data as a single array for the
            // purpose of decomposing the float into a integer part and a
            // fraction (decimal) part.
            //
            for(ui32Idx = 0; ui32Idx < 9; ui32Idx++)
            {
                //
                // Conver float value to a integer truncating the decimal part.
                //
                i32IPart[ui32Idx] = (int32_t) pfData[ui32Idx];

                //
                // Multiply by 1000 to preserve first three decimal values.
                // Truncates at the 3rd decimal place.
                //
                i32FPart[ui32Idx] = (int32_t) (pfData[ui32Idx] * 1000.0f);

                //
                // Subtract off the integer part from this newly formed decimal
                // part.
                //
                i32FPart[ui32Idx] = i32FPart[ui32Idx] -
                                    (i32IPart[ui32Idx] * 1000);

                //
                // make the decimal part a positive number for display.
                //
                if(i32FPart[ui32Idx] < 0)
                {
                    i32FPart[ui32Idx] *= -1;
                }
            }

            //
            // Print the acceleration numbers in the table.
            //
            UARTprintf("\033[5;17H%3d.%03d", i32IPart[0], i32FPart[0]);
            UARTprintf("\033[5;40H%3d.%03d", i32IPart[1], i32FPart[1]);
            UARTprintf("\033[5;63H%3d.%03d", i32IPart[2], i32FPart[2]);

            //
            // Print the angular velocities in the table.
            //
            UARTprintf("\033[7;17H%3d.%03d", i32IPart[3], i32FPart[3]);
            UARTprintf("\033[7;40H%3d.%03d", i32IPart[4], i32FPart[4]);
            UARTprintf("\033[7;63H%3d.%03d", i32IPart[5], i32FPart[5]);

            //
            // Print the Eulers in the table.
            //
            UARTprintf("\033[9;17H%3d.%03d", i32IPart[6], i32FPart[6]);
            UARTprintf("\033[9;40H%3d.%03d", i32IPart[7], i32FPart[7]);
            UARTprintf("\033[9;63H%3d.%03d", i32IPart[8], i32FPart[8]);
        }
    }
}
