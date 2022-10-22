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
//#define F_CPU 20000000

#include "mcc_generated_files/system/system.h"
#include "state.h"
#include <math.h>
#include <util/delay.h>

#define TOP_VALUE 0x007F
#define SECSLEEP   60       // Length of time between measurements
#define TIMEBASE_VALUE ((uint8_t) ceil(F_CPU * 0.000001)) // ADC timebase
/*
    Main application
*/

void PORT_init(void);
void EVSYS_init(void);
void Init_TCA(void);
void TCB_init(void);
void Init_PIT(void);
void Init_ADC(void);

    
int main(void)
{
    //Initialize the ATtiny1627
    SYSTEM_Initialize();
    
    
    
    //Baremetal Setup for State Machine
    PORT_init();
    EVSYS_init();
    Init_TCA();
    TCB_init();
    Init_PIT();
    Init_ADC();
    
    
    sei();

    //Initialize TWI Host Driver
    TWI_initHost();
    
    //Initialize the Weather Click
    WeatherClick_initialize();
    
    printf("resetting...\n");
    
    LR2_PWR_SetLow();
     
    while(1)
    { 
        stateMachine();
        //getMoistureMeasurement();
        //_delay_ms(60000);
        //WeatherClick_readSensors();
        //_delay_ms(1000);
    }
    return 0;
}

void PORT_init(void)
{
    //Set Up LED
    PORTC.DIR |= PIN0_bm;
    PORTC.OUT |= PIN0_bm;
    
    //Set Up Switch 0
    PORTC.DIR &= ~(PIN1_bm);
    PORTC.PIN1CTRL |= PORT_PULLUPEN_bm;
    
    // Set PWM Port
    PORTB.DIRSET = PIN4_bm; // PB4 TCA WO1 ALT1 out
    
    // Testing TCB1
    PORTA.DIR |= PIN6_bm;
    PORTA.OUTSET |= PIN6_bm;        // Testing Power issue
    
}

void EVSYS_init(void)
{
    EVSYS.CHANNEL3 = EVSYS_CHANNEL3_PORTC_PIN1_gc;
    EVSYS.USERTCB1CAPT = EVSYS_USER_CHANNEL3_gc;
}

// TCA produces ~868kHz PWM for measuring soil moisture
void Init_TCA(void)
{
    PORTMUX.TCAROUTEA = PORTMUX_TCA01_ALT1_gc;
    TCA0.SINGLE.CMP1 = 2; //    11;
    TCA0.SINGLE.CNT = 0x0;
    TCA0.SINGLE.CTRLB = TCA_SINGLE_CMP1_bm | TCA_SINGLE_WGMODE_SINGLESLOPE_gc;
    // (3.333333MHz / 868kHz) - 1 = 2.8
    TCA0.SINGLE.PER = 3; 
    TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV1_gc | TCA_SINGLE_ENABLE_bm;
}

void TCB_init(void)
{
    //For 1 ms Timer
    TCB0.CCMP = 10000;  // 10MHz / 1kHz = 20000
    TCB0.CTRLB = TCB_CCMPEN_bm; // ENABLE WAVEFORM OUTPUT (SHOULD BE PA5)
    TCB0.INTCTRL = TCB_CAPT_bm; // ENABLE CAPTURE INTERRUPT
    TCB0.CTRLA = TCB_CLKSEL_DIV2_gc | TCB_ENABLE_bm; // START TIMER
    
    //For button debouncing
    TCB1.CCMP = TOP_VALUE;              //In single shot so CCMP stores TOP // TOP_VALUE
    TCB1.CTRLB = TCB_CNTMODE_SINGLE_gc;
    TCB1.EVCTRL = TCB_CAPTEI_bm | TCB_EDGE_bm;  // Input capture event, 
    TCB1.CTRLA = TCB_CLKSEL_0_bm | TCB_ENABLE_bm;   // CLK_PER and TCB Enable  changed from TCB_CLKSEL_0_bm
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

void Init_ADC(void) {
    // Pin PA7 disable input buffer and turn off pullup
    PORTA.PIN7CTRL = ~PORT_PULLUPEN_bm | PORT_ISC_INPUT_DISABLE_gc;
    
    // 1. Enable the ADC by writing a ?1? to the ENABLE bit in the Control A (ADCn.CTRLA) register.
    ADC0.CTRLA = ADC_ENABLE_bm;
    // 2. Configure the Prescaler (PRESC) bit field in the Control B (ADCn.CTRLB) register.
    ADC0.CTRLB = ADC_PRESC_DIV4_gc; // ADC CLK 5MHz
    
    // 3. Configure the Timebase (TIMEBASE) and Reference Select (REFSEL) bit fields in the Control C (ADCn.CTRLC) register.
    // timebase to get 1us (with ADC CLK = 5Mhz) is 20 (0x14) -- SEE #define
    // and use VDD as ADC ref
    ADC0.CTRLC = (TIMEBASE_VALUE << ADC_TIMEBASE_gp) | ADC_REFSEL_VDD_gc;
    
    // 4. Configure the Sample Duration (SAMPDUR) bit field in the Control E (ADCn.CTRLE) register.
    ADC0.CTRLE = 0xFF;
    
    // select AIN7 (PA7) as ADC input
    ADC0.MUXPOS = ADC_MUXPOS_AIN7_gc;
    
    // setup single 12-bit read and start ADC read immediately
    ADC0.COMMAND = ADC_MODE_SINGLE_12BIT_gc;
}



