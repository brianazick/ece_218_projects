/* 
 * File: main_project_3.c
 * Author: B. Zick
 * Date: 2019-02-13
 * Purpose: Run a DC Motor with a PIC24
 * Modified:
 */
/*********** COMPILER DIRECTIVES *********/

// #include for textbook library header files
#include "pic24_all.h"
#include "lcd4bit_lib.h"

// #defines for handy constants 
#define EN_12 (_RB12) // RB12 (pin 23) is connected to EN_12 on the motor driver
#define PWM_PERIOD 510 //Define previously calculated period in ticks
#define p_min 255 //Define previous calculated minimum pulse width in ticks
#define p_max 510 //Define previous calculated maximum pulse width in ticks



/*********** GLOBAL VARIABLE AND FUNCTION DEFINITIONS *******/
volatile uint16_t A1;
volatile uint16_t A2;

double angular_speed;
char angular_speed_ascii[6]; //declare an array for the motor encoder output

volatile uint16_t EdgeD;
volatile uint16_t EdgeA;
volatile uint16_t IC1_period;

void convertRevToASCII(void) {
    /* Follow the algorithm laid out in the exercise 3 manual
     * Slightly adapted, multiplying by 10 instead of 100 provided more consistent result
     */
    uint16_t nint;

    angular_speed_ascii[0] = 0x30; //initialize to 0
    angular_speed_ascii[1] = 0x30; //initialize to 0
    angular_speed_ascii[2] = 0x30; //initialize to 0
    angular_speed_ascii[3] = 0x2E; //initialize to period
    angular_speed_ascii[4] = 0x30; //initialize to 0
    angular_speed_ascii[5] = 0x30; //initialize to 0

    nint = angular_speed * 100;

    while (nint > 9999){
        // count the number of '10000s' and increment the ascii value accordingly
        nint -= 10000;
        angular_speed_ascii[0] += 0x01;
    }
    
    while (nint > 999){
        // count the number of '1000s' and increment the ascii value accordingly
        nint -= 1000;
        angular_speed_ascii[1] += 0x01;
    }
    
    while (nint > 99){
        // count the number of '100s' and increment the ascii value accordingly
        nint -= 100;
        angular_speed_ascii[2] += 0x01;
    }

    while (nint > 9){
        // count the number of '10s' and increment the ascii value accordingly
        nint -= 10;
        angular_speed_ascii[4] += 0x01;
    }

    while (nint > 0){
        // count the number of '1s' and increment the ascii value accordingly
        nint -= 1;
        angular_speed_ascii[5] += 0x01;
    }
}

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
    CONFIG_OC2_TO_RP(RB2_RP); //Map the OC2 output to the remappable pin RP2
    OC2RS = 0x0000; //clear the register
    OC2R = 0x0000; //clear the register
    OC2CONbits.OCM = 0x6; //PWM mode on OC2, fault pin disabled
    OC2CONbits.OCTSEL = 0; //use timer2 as the clock source
}

void configIC1(void) {
   CONFIG_IC1_TO_RP(RB10_RP);  //Map IC1 to RB10 (pin 21)
   T3CONbits.TON = 0;  //Disable Timer3 when writing to IC control registers
   IC1CONbits.ICTMR = 0; //use timer 3    
   IC1CONbits.ICM = 0b011; //capture every rising edge.
   IC1CONbits.ICI = 0b00; //interrupt every capture 
} 

void configTimer2(void) {
    T2CON = 0x0010; //TMR2 off, default idle mode, gate off, prescale 1:8, 32-bit mode off, internal clock
    PR2 = PWM_PERIOD; //delay = PWM_PERIOD
    TMR2 = 0x0000; //clear the timer
    _T2IF = 0; //set the initial state to off
}

void configTimer3(void){
    T3CONbits.TON = 0;  //Disable Timer3 when writing to IC control registers  
    T3CONbits.TCKPS=0b01;  //prescale 1:8 - Timer 3 clock period = 0.2us
    PR3 = 0xFFFF;            //Maximum Timer3 interval
    T3CONbits.TCS = 0;     //(instruction clock = Tcy)
    TMR3 = 0;  
    _T3IF = 0;
} 

void _ISR _IC1Interrupt(void)  
{
  EdgeD = IC1BUF;  //Read the IC1 buffer to capture the time of the most recent edge
  IC1_period = EdgeD - EdgeA; //Calculate the counts in one period by subtracting  
  //most recent edge from the previous edge.
  EdgeA = EdgeD; //Store the most recent edge as the previous edge
  _IC1IF = 0;  //Clear the IC1 interrupt flag so that interrupts can happen again
}

void _ISR _T2Interrupt(void){
    uint16_t potentiometer_value;
    uint16_t p_diff;
    p_diff = p_max - p_min;
    
    potentiometer_value = convertADC1(); //read in the ADC value
    if (potentiometer_value < 501) {
        potentiometer_value = 500 - potentiometer_value;
        A1 = ((potentiometer_value/500.0)*(p_diff)) + p_min;
        A2 = 0;
    } else if (potentiometer_value < 523) {
        A1 = 0;
        A2 = 0;
    } else {
        potentiometer_value = potentiometer_value - 524;
        A1 = 0;
        A2 = ((potentiometer_value/500.0)*(p_diff)) + p_min;
    }
    
    OC1RS = A1; //update the OC1RS register
        
    OC2RS = A2; //update the OC2RS register

    _T2IF = 0; //set the flag bit to 0
}

    
/********** MAIN PROGRAM ********************************/
int main ( void )  //main function that....
{ 
/* Define local variables */
    uint16_t pulse_period;
    double temp;

/* Call configuration routines */
    configClock(); //Sets the clock to 40MHz using FRC and PLL
    configControlLCD(); //Configure the RS, RW, and E control lines and outputs and initialize them low
    configOC1();
    configOC2();
    configIC1();
    configTimer2();
    configTimer3();
    configUART1(230400); //Configure UART1 for 230400 baud


/* Initialize ports and other one-time code */
    uint8_t clear_terminal[] = "\e[1;1H\e[2J"; //String to clear terminal
    outString(clear_terminal); //Clear the terminal
    uint8_t u8_str[] = "Serial communication has been established.\n\r"; //Initialize welcome message
    outString(u8_str); //Print welcome message

    initLCD(); //Execute the initialization sequence for the LCD display
    
    CONFIG_RB12_AS_DIG_OUTPUT();
    
    CONFIG_RA1_AS_ANALOG(); //sets RA1 (pin 3) to be analog
    configADC1_ManualCH0(RA1_AN, 31, 0); //set up ADC for potentiometer
    
    A1 = p_min;
    A2 = 0;
    _LATB12 = 1;
    
    _IC1IE = 1; //enable input compare interrupts
    T3CONbits.TON = 1; //Turn the timer on
    
    _T2IE = 1; //set timer interrupt enable
    _T2IF = 0; //clear the timer flag
    T2CONbits.TON = 1; //Turn the timer on
    
    
    
/* Main program loop */
	while (1) {
        pulse_period = (IC1_period*0.2); // pulse_period is in microseconds
        printf("\n\r");
        printf("IC1_period is %5d ticks \n\r", IC1_period);
        printf("Pulse period is %5d microseconds \n\r", pulse_period);
        
        temp = pulse_period;
        temp = temp/1000; //convert to milliseconds
        temp = temp/1000; //convert to seconds
        
        printf("Pulse period is %4.5f seconds \n\r", temp);
        
        angular_speed = 1.0/(temp * 52.8);
        
        printf("Angular speed is %4.5f rev/s \n\r", angular_speed);
        
        convertRevToASCII();
        
        writeLCD(0x80, 0, 1, 1); //reset the cursor to top leftmost char
        outStringLCD("Angular speed is");
        writeLCD(0xC0, 0, 1, 1); //reset the cursor to bottom leftmost char in 
        outStringLCD(angular_speed_ascii);
        writeLCD(0xC8, 0, 1, 1); //reset the cursor to bottom 6th char in 
        outStringLCD("rev/s");
       
        DELAY_MS(1500);
	}
}