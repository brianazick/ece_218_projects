/*
 * File: main_temperature_alarm.c
 * Author: I. Krause and B. Zick
 * Date: 2019-01-18
 * Purpose: ECE-218 W19 Project 1
 * Modified:
 */
/*********** COMPILER DIRECTIVES *********/

// #include for textbook library header files
#include "pic24_all.h"

// #defines for handy constants
#define Buzzer (_LATA0)  // LED on microstick, RA0 (pin 2)
#define Button (_RB2) // Active low button attached to RB2 (pin 6)

/*********** GLOBAL VARIABLE AND FUNCTION DEFINITIONS *******/

enum States {disarmed, button_pressed_0, button_released_0, alarm, button_pressed_1, button_released_1} state; //define global state variable
char temp_f_ascii[4]; //define an array for ascii output
float temp_f; //define a variable to hold the current temperature in fahrenheit

void Tickfct () {  //Define state machine
    //local function variables

    switch (state) {    //transition switch statement
        case disarmed:  //initialize state
            if (Button == 0){
                state = button_pressed_0; //move to the first button pressed state if the button is pressed
            }else if (Button == 1){
                state = disarmed; //otherwise stay in this state
            }
            break;
        case button_pressed_0:
            if (Button == 1){
                state = button_released_0; //move to the first button released if the button is released
            }else if (Button == 0){
                state = button_pressed_0; //otherwise stay in this state
            }
            break;
        case button_released_0:
            if (temp_f >= 90){
                state = alarm; //move to the alarm state if the temperature reaches about 90 degress fahrenheit
            }else {
                state = button_released_0; //otherwise stay in this state
            }
            break;
        case alarm:
            if (Button == 0){
                state = button_pressed_1; //move to the second button pressed state if the button is pressed
            }else if (Button == 1){
                state = alarm; //otherwise stay in this state
            }
            break;
        case button_pressed_1:
            if (Button == 1){
                state = button_released_1; //move to the second button released state if the button is released
            }else if (Button == 0){
                state = button_pressed_1; //otherwise stay in this state
            }
            break;
        case button_released_1:
            state = disarmed; //this state requires not input to transition
            break;
        default:
            state = disarmed; //default behavior is to go to the disarmed/reset state
            break;
    }
    switch (state) {    //output switch statement
        case disarmed:  //initialize state
            outString("DISARMED ");
            outString(temp_f_ascii); //split the printing function into three to allow printing of a variable value
            outString(" degrees F\n\r"); //move the cursor to the beginning of the next line
            Buzzer = 0; //initialize the buzzer to the off state
            break;
        case button_pressed_0:
            break;
        case button_released_0:
            outString("ARMED ");
            outString(temp_f_ascii);
            outString(" degrees F\n\r");
            break;
        case alarm:
            outString("ALARM! Press Button to Stop!\n\r");
            Buzzer = 1; //turn the buzzer on when in the alarm state
            break;
        case button_pressed_1:
            break;
        case button_released_1:
            break;
        default:
            outString("DISARMED ");
            outString(temp_f_ascii);
            outString(" degrees F\n\r");
            Buzzer = 0;
            break;
    }
 }

void convertTempToASCII () {
    /* Follow the algorithm laid out in the exercise 3 manual
     * Slightly adapted, multiplying by 10 instead of 100 provided more consistent result
     */
    uint16_t nint;

    temp_f_ascii[0] = 0x30; //initialize to 0
    temp_f_ascii[1] = 0x30; //initialize to 0
    temp_f_ascii[2] = 0x2E; //initialize to period
    temp_f_ascii[3] = 0x30; //initialize to 0

    nint = temp_f * 10;

    while (nint > 99){
        // count the number of '100s' and increment the ascii value accordingly
        nint -= 100;
        temp_f_ascii[0] += 0x01;
    }

    while (nint > 9){
        // count the number of '10s' and increment the ascii value accordingly
        nint -= 10;
        temp_f_ascii[1] += 0x01;
    }

    while (nint > 0){
        // count the number of '1s' and increment the ascii value accordingly
        nint -= 1;
        temp_f_ascii[3] += 0x01;
    }
}

/********** MAIN PROGRAM ********************************/
int main ( void )  //main function that carries through the temperature alarm
{
/* Define local variables */
  uint16_t u16_var;
  float millivolts;
  float temp_c;

/* Call configuration routines */
  configClock();  //Sets the clock to 40MHz using FRC and PLL
  configUART1(230400); //parameter is the baudrate for serial communication
  CONFIG_RA1_AS_ANALOG(); //sets RA1 (pin 3) to be analog
  configADC1_ManualCH0(RA1_AN, 31, 0); //set up ADC
  CONFIG_RA0_AS_DIG_OUTPUT(); //Buzzer on RA0 (pin 2)
  CONFIG_RB2_AS_DIG_INPUT(); //pushbutton switch on RB2 (pin 6)

/* Initialize ports and other one-time code */
  uint8_t clear_terminal[] = "\e[1;1H\e[2J"; //String to clear terminal
  outString(clear_terminal); //Clear the terminal
  uint8_t u8_str[] = "Serial communication has been established.\n\r"; //Initialize welcome message
  outString(u8_str); //Print welcome message
  state = disarmed; //Initialize the state machine to the disarmed/reset state

/* Main program loop */
	while (1) {
        u16_var = convertADC1();
        millivolts = (float)u16_var*(((float)980 - (float)816)/(float)1023) + (float)816; //use the template of a transfer function to convert the analog reading to millivolts
        temp_c = (millivolts/(float)-5.46) + (float)189.487; //derived from the two point equation on the datasheet, decided on a range from 10 C to 40 C
        temp_f = (temp_c * ((float)9/(float)5)) + 32; //convert from celsius to fahrenheit
        convertTempToASCII();
        Tickfct();
        DELAY_MS(100);
	}
}
