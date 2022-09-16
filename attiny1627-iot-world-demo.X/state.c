#include "state.h"
#include <stdio.h>
#define READ_INTERVAL 180;  //Time between sensor readings

#define UI_BUTTON_FLAG              (1 << 0)
#define TTN_JOIN_SUCCESS_FLAG       (1 << 1)
#define TTN_JOIN_ERROR_FLAG         (1 << 2)
#define TX_SUCCESS_FLAG             (1 << 3)
#define TX_ERROR_FLAG               (1 << 4)
#define SLEEP_TIMER_FLAG            (1 << 5)


typedef enum    {
    TTN_NOT_JOINED,
    TTN_JOIN_REQUEST,
    TTN_JOINED,
    SEND_DATA,
    SLEEP
} STATE;

static STATE state = TTN_NOT_JOINED;
static volatile uint16_t event_flags = 0;


volatile uint16_t secs = 0;
uint16_t numTx = 0;

//LR2 payload for TTN - String of hex characters representing sensor data
char payload[15];

//Data structure: measurements from Weather Click and moisture sensor
static sensor_data_t data;  


ISR(TCB0_INT_vect)
{
    if (PORTC.IN & PIN4_bm)
    {
        BUTTON_releaseCallback();
    }
    else
    {
        BUTTON_pressCallback();
    }

    TCB0.INTFLAGS = TCB_CAPT_bm;
}

ISR(RTC_PIT_vect)   {
    secs++;
    
    RTC.PITINTFLAGS |= RTC_PI_bm;
}


void BUTTON_releaseCallback(void)
{
    //PORTB.OUTSET |= PIN7_bm;
    
    event_flags |= UI_BUTTON_FLAG;
}

void BUTTON_pressCallback(void)
{
    //PORTB.OUTCLR |= PIN7_bm;   
}

uint16_t getNumTx() {
    return numTx;
}

void stateMachine()
{
    //Handle User Button Presses
    if(event_flags & UI_BUTTON_FLAG) {
        switch(state) {
            case TTN_NOT_JOINED:
                //Initialize the LR2
                LR2_reset();

                //Configure the LR2 with Necessary Keys
                LR2_config();
                
                //Send Join Request
                LR2_join();
                
                state = TTN_JOIN_REQUEST; 
                 
                break;
                
            case TTN_JOINED:
                state = SEND_DATA;
                break;
                
            default:
                break;
        }
        event_flags &= ~UI_BUTTON_FLAG;
    }
   
    switch(state) {
        case TTN_JOIN_REQUEST:
            //TODO: Handle Join Errors
            if(Buffer_find("accepted\r\n")) {
                USB_sendStringRaw("Rx: accepted\n");
            }
            //For now automatically continues without checking for join errors
            state = TTN_JOINED;
            break;
            
        //Read measurements, prepare payload for TTN, every 10th payload is
        // is sent as confirmed, otherwise send as unconfirmed, After transmitting,
        // start a timer     
        case SEND_DATA:
            //Gather and format sensor data for transmission
            getSensorData(&data);
            formatPayload(payload, &data);
            if(numTx % 10 == 0) {
                LR2_tx_cnf(payload);
            }
            else {
                LR2_tx_uncnf(payload);
            }
            //Advance Transmission Count
            numTx++;
            //Start the Timer
                RTC.CTRLA |= RTC_RTCEN_bm;
            state = SLEEP;
            
        //Wait for mac_tx_ok, then go to sleep/ enter low power mode 
        //TODO: parse LR2 messages, if mac_tx_ok, sleep, otherwise handle error
        case SLEEP:
            //After 3 minutes, return to read and transmit next measurement
            if(secs == 180 ) {
                state = SEND_DATA;
                //Stop the timer
                RTC.CTRLA &= ~RTC_RTCEN_bm;
                secs = 0;
            }
            //Allow terminal access for debug purposes, ie, type mac get dr to
            //check data rate, or mac get status to check if joined
            //Remove when not debugging
            sendAndReceiveBuffers();
            break;
        default:
            break;
            
    }

}






