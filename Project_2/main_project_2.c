/*
 * File: main_project_2.c
 * Author: A. Bruno and B. Zick
 * Date: 2019-01-30
 * Purpose: Control multiple servo motors with a potentiometer and a slider switch
 * Modified:
 */
/*********** COMPILER DIRECTIVES *********/

// #include for textbook library header files
#include "pic24_all.h"
#include "lcd4bit_lib.h"

// #defines for handy constants
#define Slider (_RB2) // Slider switch attached to RB2 (pin 6)
#define PWM_PERIOD 3125 //Define previously calculated period in ticks
#define positional_p_min 55 //Define previous calculated minimum pulse width in ticks
#define positional_p_mid 156 //Define previous calculated minimum pulse width in ticks
#define positional_p_max 385 //Define previous calculated middle pulse width in ticks
#define continuous_p_min 203 //Define previous calculated minimum pulse width in ticks
#define continuous_p_mid 232 //Define previous calculated middle pulse width in ticks
#define continuous_p_max 269 //Define previous calculated maximum pulse width in ticks



/*********** GLOBAL VARIABLE AND FUNCTION DEFINITIONS *******/
volatile uint16_t pulse_width_pos; //volatile because it will frequently be changed
volatile uint16_t pulse_width_con; //volatile because it will frequently be changed

void configOC1(void) {
    T2CONbits.TON = 0; //Turn the timer off
    CONFIG_OC1_TO_RP(RB1_RP); //Map the OC1 output to the remappable pin RP1
    OC1RS = 0x0000; //clear the register
    OC1R = 0x0000; //clear the register
    OC1CONbits.OCM = 0x6; //PWM mode on OC1, fault pin disabled
    OC1CONbits.OCTSEL = 0; //use timer2 as the clock source
}

void configOC2(void) {
    T2CONbits.TON = 0; //Turn the timer off
    CONFIG_OC2_TO_RP(RB0_RP); //Map the OC2 output to the remappable pin RP0
    OC2RS = 0x0000; //clear the register
    OC2R = 0x0000; //clear the register
    OC2CONbits.OCM = 0x6; //PWM mode on OC2, fault pin disabled
    OC2CONbits.OCTSEL = 0; //use timer2 as the clock source
}

void configTimer2(void) {
    T2CON = 0x0030; //TMR2 off, default idle mode, gate off, prescale 1:256, 32-bit mode off, internal clock
    PR2 = PWM_PERIOD; //delay = PWM_PERIOD
    TMR2 = 0x0000; //clear the timer
    _T2IF = 0; //set the initial state to off
}

void _ISR _T2Interrupt(void){
    /* The if statement is in here because these operations only need to occur
     * every the timer goes off, not every iteration of the main loop
     */
    if (Slider == 1) {
        OC1RS  = pulse_width_con; //update the OC1RS register
        writeLCD(0x80, 0, 1, 1); //reset the cursor to top leftmost char
        outStringLCD("Continuous"); //write string output
    } else {
        OC2RS  = pulse_width_pos; //update the OC1RS register
        writeLCD(0x80, 0, 1, 1); //reset the cursor to top leftmost char
        outStringLCD("Positional"); //write string output
    }

    _T2IF = 0; //set the flag bit to 0
}



/********** MAIN PROGRAM ********************************/
int main ( void )  //main function that....
{
/* Define local variables */
    uint16_t u16_var;
    float voltage;

/* Call configuration routines */
	configClock();  //Sets the clock to 40MHz using FRC and PLL

    configControlLCD(); //Configure the RS, RW, and E control lines and outputs and initialize them low

    configTimer2(); //Sets the timer to desired settings

    configOC1(); //Config output compare 1
    configOC2(); //Config output compare 2

    CONFIG_RA1_AS_ANALOG(); //sets RA1 (pin 3) to be analog
    configADC1_ManualCH0(RA1_AN, 31, 0); //set up ADC

    CONFIG_RB2_AS_DIG_INPUT(); //slider switch on RB2 (pin 6)

/* Initialize ports and other one-time code */
    _T2IE = 1; //set timer interrupt enable
    _T2IF = 0; //clear the timer flag
    T2CONbits.TON = 1; //Turn the timer on

    initLCD(); //Execute the initialization sequence for the LCD display

/* Main program loop */
	while (1) {
        u16_var = convertADC1(); //read in the ADC value
        voltage = ((float)u16_var/(float)1023)*3.3; //convert the ADC reading to the appropriate voltage
        pulse_width_pos = (float)voltage*100 + 55; //convert the voltage of the potentiometer to the pulse_width_pos range
        pulse_width_con = (float)voltage*20 + 203; //convert the voltage of the potentiometer to the pulse_width_con range
    }
}
