 /*
 * MAIN Generated Driver File
 * 
 * @file main.c
 * 
 * @defgroup main MAIN
 * 
 * @brief This is the generated driver implementation file for the MAIN driver.
 *
 * @version MAIN Driver Version 1.0.0
*/

/*
© [2022] Microchip Technology Inc. and its subsidiaries.

    Subject to your compliance with these terms, you may use Microchip 
    software and any derivatives exclusively with Microchip products. 
    You are responsible for complying with 3rd party license terms  
    applicable to your use of 3rd party software (including open source  
    software) that may accompany Microchip software. SOFTWARE IS ?AS IS.? 
    NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS 
    SOFTWARE, INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT,  
    MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT 
    WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, 
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY 
    KIND WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF 
    MICROCHIP HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE 
    FORESEEABLE. TO THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP?S 
    TOTAL LIABILITY ON ALL CLAIMS RELATED TO THE SOFTWARE WILL NOT 
    EXCEED AMOUNT OF FEES, IF ANY, YOU PAID DIRECTLY TO MICROCHIP FOR 
    THIS SOFTWARE.
*/
#include "mcc_generated_files/system/system.h"
#include "state.h"

#define TOP_VALUE 0xFFFF
#define SECSLEEP   60       // Length of time between measurements
/*
    Main application
*/

void PORT_init(void);
void EVSYS_init(void);
void TCB_init(void);
void Init_PIT(void);
    
int main(void)
{
    //Initialize the ATtiny1627
    SYSTEM_Initialize();
    
    //Baremetal Setup for State Machine
    //The attiny3236 version also includes ADC (for soil probe and battery) and TCA (for PWM))
    PORT_init();
    EVSYS_init();
    TCB_init();
    Init_PIT();
    
    sei();

    //Initialize TWI Host Driver
    TWI_initHost();
    
    //Initialize the Weather Click
    WeatherClick_initialize();
    
    //Turn on LED (Active Low) Initialization Complete
    PORTB.OUT &= ~PIN7_bm;
    
    while(1)
    { 
        stateMachine();
    }
    return 0;
}

void PORT_init(void)
{
    //Set Up LED
    PORTB.DIR |= PIN7_bm;
    
    //Ensure LED Off to Start
    PORTB.OUT |= PIN7_bm;
    
    //Set Up Switch 0
    PORTC.DIR &= ~(PIN4_bm);
    PORTC.PIN4CTRL |= PORT_PULLUPEN_bm;
}

void EVSYS_init(void)
{
    EVSYS.CHANNEL3 = EVSYS_CHANNEL3_PORTC_PIN4_gc;
    EVSYS.USERTCB1CAPT = EVSYS_USER_CHANNEL3_gc;
}

void TCB_init(void)
{
    //For 1 ms Timer
    TCB0.CCMP = 10000;  // 10MHz / 1kHz = 20000
    TCB0.CTRLB = TCB_CCMPEN_bm; // ENABLE WAVEFORM OUTPUT (SHOULD BE PA5)
    TCB0.INTCTRL = TCB_CAPT_bm; // ENABLE CAPTURE INTERRUPT
    TCB0.CTRLA = TCB_CLKSEL_DIV2_gc | TCB_ENABLE_bm; // START TIMER
    
    //For button debouncing
    TCB1.CCMP = TOP_VALUE;              //In single shot so CCMP stores TOP 
    TCB1.CTRLB = TCB_CNTMODE_SINGLE_gc;
    TCB1.EVCTRL = TCB_CAPTEI_bm | TCB_EDGE_bm;  // Input capture event, 
    TCB1.CTRLA = TCB_CLKSEL_0_bm | TCB_ENABLE_bm;   // CLK_PER and TCB Enable
    TCB1.CNT = TOP_VALUE;
    TCB1.INTCTRL = TCB_CAPT_bm; //Capture Interrupt Enable
}

void Init_PIT(void) {
/*  1. Configure the RTC clock CLK_RTC as described in section 23.4.1.1. Configure the Clock CLK_RTC.
    2. Enable the interrupt by writing a ?1? to the Periodic Interrupt (PI) bit in the PIT Interrupt Control
    (RTC.PITINTCTRL) register.
    3. Select the period for the interrupt by writing the desired value to the Period (PERIOD) bit field in the Periodic
    Interrupt Timer Control A (RTC.PITCTRLA) register.
    4. Enable the PIT by writing a ?1? to the Periodic Interrupt Timer Enable (PITEN) bit in the RTC.PITCTRLA
    register.
 * */
    RTC.CLKSEL = RTC_CLKSEL_INT1K_gc;//RTC_CLKSEL_INT32K_gc;
    RTC.PITINTCTRL = RTC_PI_bm;
    while(RTC.PITSTATUS & RTC_CTRLBUSY_bm); //wait until not busy to write
    RTC.PITCTRLA = RTC_PERIOD_CYC1024_gc | RTC_PITEN_bm; //RTC_PERIOD_CYC32768_gc | RTC_PITEN_bm;
    
}
