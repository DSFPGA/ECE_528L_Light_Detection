/**
 * @file main.c
 *
 * @brief Main source code for the Light_Detection (I2C) program.
 *
 * This file contains the main entry point for the Light_Detection (I2C) program.
 *
 * It establishes communication with the following peripheral using I2C to demonstrate light detection:
 *  - OPT3001 Ambient Light Sensor (TI BP-BASSENSORSMKII BoosterPack)
 *
 * The following Timer_A modules are used in this lab:
 *  - Timer_A0: Used to generate PWM signals that will be used to drive the DC motors
 *  - Timer_A1: Used to generate periodic interrupts at a specified rate (1 kHz)
 *
 * @author Dominic Sanqui
 */

#include "msp.h"
#include <math.h>
#include <stdint.h>

#include "inc/Clock.h"
#include "inc/CortexM.h"
#include "inc/GPIO.h"
#include "inc/EUSCI_A0_UART.h"
#include "inc/Motor.h"
#include "inc/Timer_A1_Interrupt.h"
#include "inc/OPT3001.h"
#include "inc/GPIO.h"

// Enable DEBUG_ACTIVE to view the output of the prinf statements
#define DEBUG_ACTIVE 1

// Initialize lux threshold. Must be modified based on light intensity
#define LUX_THRESHOLD 5000

// Declare struct for the light sensor
OPT3001_Result Light;

// Declare global variable to store lux value
static double lux = 0;

// Declare global variable to indicate that lux has been measured by the light sensor
static uint8_t lux_done = 0;

// Set initial duty cycle of the left wheel to 25%
static uint16_t Duty_Cycle_Left = 3750;

// Set initial duty cycle of the right wheel to 25%
static uint16_t Duty_Cycle_Right = 3750;

// Maximum safe duty cycle at 100%
static uint16_t Duty_Cycle_Max = 15000;

// Maximum lux value measured by OPT3001 Sensor
static uint32_t lux_max = 80000;

// Minimum lux value needed to move the motors
static uint16_t lux_min = 10000;

/*
 * @brief This function samples the OPT3001 Ambient Light Sensor and calculates the lux
 *
 * @param None
 *
 * @return None
 */
void Sample_Light_Sensor();

/*
 * @brief This function enables the motors when the measured lux is greater than the specific threshold
 *
 * @param None
 *
 * @return None
 */
void Controller_1();

/*
 * @brief User-defined function executed by Timer A1 using a periodic interrupt
 *
 * @param None
 *
 * @return None
 */
void Timer_A1_Periodic_Task(void);

/* @brief Function updated for task section. Moves or stops the robot based on the
 * value of the lux measured from the OPT3001 sensor
 *
 * The function first verifies that the measured lux from the OPT3001 sensor is
 * greater than the minimum lux threshold. If it is not, stop the motors. If it
 * is, calculate the percentage between the measured lux
 * and the maximum lux value that can be measured. Based on the percentage,
 * the motors are provided with a duty cycle that ranges from 10% to 60%
 *
 * @param None
 *
 * @return None
 */

void Controller_1()
{

    if (lux >= lux_min)
    {
        LED1_Output(RED_LED_OFF);

        double lux_percentage = (lux / lux_max) * 100;

        double motor_speed = 0;

        if ((lux_percentage >= 25) & (lux_percentage < 35))
        {
            motor_speed = Duty_Cycle_Max * 0.20;
            Motor_Forward(motor_speed, motor_speed);
            LED2_Output(RGB_LED_YELLOW);
        }

        else if ((lux_percentage >= 35) & (lux_percentage < 45))
        {
            motor_speed = Duty_Cycle_Max * 0.30;
            Motor_Forward(motor_speed, motor_speed);
            LED2_Output(RGB_LED_PINK);
        }

        else if ((lux_percentage >= 45) & (lux_percentage < 55))
        {
            motor_speed = Duty_Cycle_Max * 0.40;
            Motor_Forward(motor_speed, motor_speed);
            LED2_Output(RGB_LED_SKY_BLUE);
        }

        else if ((lux_percentage >= 55) & (lux_percentage < 65))
        {
            motor_speed = Duty_Cycle_Max * 0.50;
            Motor_Forward(motor_speed, motor_speed);
            LED2_Output(RGB_LED_WHITE);
        }

        else if (lux_percentage >= 65)
        {
            motor_speed = Duty_Cycle_Max * 0.60;
            Motor_Forward(motor_speed, motor_speed);
            LED2_Output(RGB_LED_GREEN);
        }

        else
        {
            motor_speed = Duty_Cycle_Max * 0.10;
            Motor_Forward(motor_speed, motor_speed);
            LED2_Output(RGB_LED_BLUE);
        }
    }

    else
    {
        Motor_Stop();
        LED2_Output(RGB_LED_RED);
        LED1_Output(RED_LED_ON);
    }
}

/*
void Controller_1()
{
    if (lux >= LUX_THRESHOLD)
    {
        Motor_Forward(Duty_Cycle_Left, Duty_Cycle_Right);
    }

    else
    {
        Motor_Stop();
    }
}
*/


void Timer_A1_Periodic_Task(void)
{
    Controller_1();
}

void Sample_Light_Sensor()
{
    Light = OPT3001_Read_Light();
    lux = (0.01) * (pow(2, Light.Exponent)) * (Light.Result);
}

int main(void)
{

    // Initialize the 48 MHz Clock
    Clock_Init48MHz();

    // Ensure the interrupts are disabled during initialization
    DisableInterrupts();

    // Initialize the built in red LED and the RGB LEDs
    LED1_Init();
    LED2_Init();

    // Initialize the Chassis board LEDs
    Chassis_Board_LEDs_Init();

    // Initialize EUSCI_A0_UART to use the printf function
    EUSCI_A0_UART_Init_Printf();

    // Initialize Timer A1 with interrupts enabled and an interrupt rate of 1 kHz
    Timer_A1_Interrupt_Init(&Timer_A1_Periodic_Task, TIMER_A1_INT_CCR0_VALUE);

    // Initialize the DC motors
    Motor_Init();

    // Initialize I2C using EUSCI_B1 module
    EUSCI_B1_I2C_Init();

    P8->OUT |= 0x01;

    // Initialize the OPT3001 Ambient Light Sensor
    OPT3001_Init();

    P8->OUT &= ~0x01;

    // Enable the interrupts used by the timer modules and other peripherals
    EnableInterrupts();


    while(1)
    {
        Sample_Light_Sensor();

#ifdef DEBUG_ACTIVE
        printf("\nLight Sensor Exponent: %d\n", Light.Exponent);
        printf("Light Sensor Mantissa: %d\n", Light.Result);
        printf("Light Sensor Lux: %f\n\n", lux);
        Clock_Delay1ms(100);
#endif
    }
}
