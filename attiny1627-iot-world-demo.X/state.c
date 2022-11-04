#include "state.h"
#include <stdio.h>
#include <util/delay.h>
/**
  Section: Variable Definitions
 */

#define DEFAULT_STANDBY_TIME    BME280_STANDBY_HALFMS
#define DEFAULT_FILTER_COEFF    BME280_FILTER_COEFF_OFF
#define DEFAULT_TEMP_OSRS       BME280_OVERSAMP_X1
#define DEFAULT_PRESS_OSRS      BME280_OVERSAMP_X1
#define DEFAULT_HUM_OSRS        BME280_OVERSAMP_X1
#define DEFAULT_SENSOR_MODE     BME280_FORCED_MODE

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

bool weather_initialized = 0;
bool label_initial = false;
static STATE state = TTN_NOT_JOINED;
static volatile uint16_t event_flags = 0;
volatile uint32_t millis = 0;   // counter for elapsed milliseconds
uint32_t secs = 0;              // counter for elapsed seconds
uint32_t lastsecs = 0;          // reference to determine total elapsed time in sec
uint16_t numTx = 0;

//LR2 payload for TTN - String of hex characters representing sensor data
char payload[17];

//Data structure: measurements from Weather Click and moisture sensor
static sensor_data_t data;


// 1ms interrupt
ISR(TCB0_INT_vect)  {
    millis++;
    TCB0.INTFLAGS |= TCB_CAPT_bm;
}


ISR(TCB1_INT_vect)
{
    if (PORTC.IN & PIN4_bm)
    {
        BUTTON_releaseCallback();
    }
    else
    {
        BUTTON_pressCallback();
    }

    TCB1.INTFLAGS = TCB_CAPT_bm;
}

ISR(RTC_PIT_vect)   {
    secs++;
    
    RTC.PITINTFLAGS |= RTC_PI_bm;
}


/**
  Section: Driver APIs
 */


void WeatherClick_initialize(void) {
    BME280_reset();
    _delay_ms(50);
    BME280_readFactoryCalibrationParams();
    BME280_config(BME280_STANDBY_HALFMS, BME280_FILTER_COEFF_OFF);
    BME280_ctrl_meas(BME280_OVERSAMP_X1, BME280_OVERSAMP_X1, BME280_FORCED_MODE);
    BME280_ctrl_hum(BME280_OVERSAMP_X1);
    BME280_initializeSensor();
    weather_initialized = 1;
}

void WeatherClick_readSensors(void) {
    if (DEFAULT_SENSOR_MODE == BME280_FORCED_MODE) {
        BME280_startForcedSensing();
    }
    BME280_readMeasurements();
}

//Get 100 raw values from soil moisture sensor for calibration
void printRawValues() {
    uint8_t count = 0;
    uint16_t val;
    uint16_t min = ADC0_GetConversion(7);
    uint16_t max = min;
    
    printf("\nAfter 100 Moisture Readings\n");
    printf("-----------------------------------------------------------------------\n");
    while(count < 100) {
        val = ADC0_GetConversion(7);
        if(val < min && val != 0){
            min = val;
        }
        if(val > max) {
            max = val;
        }    
        //printf("%d\n",val);
        count++;
    }
    printf("The MAX reading was %d\n", max);
    printf("The MIN reading was %d\n\n", min);
    
    printf("The MAX maps to %d\n", map(max));
    printf("The MIN maps to %d\n", map(min));
}

uint8_t map(uint16_t raw) {    
    float val;
    
    // Account for random sporadic outlier values well out of range
    if(raw > IN_MAX) {
        raw = IN_MAX;
    }
    if(raw < IN_MIN) {
        raw = IN_MIN;
    }
    
    val = (raw - IN_MIN) * CONVERSION_PERCENT;

    return 100 - (uint8_t) val;
}

uint8_t getMoistureMeasurement() {
    uint16_t val = ADC0_GetConversion(7);
    return map(val);    
}

uint8_t getBatteryLevel()   {
    //For Testing Purposes loses 1% every 10 Tx's TODO: create better calculation
    uint16_t level = 100 - (numTx / 10); 
    return level;
}

//Gets Data from the Weather Click and Soil Moisture Sensor
void getSensorData(sensor_data_t *data)
{
    WeatherClick_readSensors();

    data->temp_air = (int8_t) BME280_getTemperature();
    data->press = BME280_getPressure();
    data->humid = (uint8_t) BME280_getHumidity();
    data->moist = getMoistureMeasurement();
    data->battery = getBatteryLevel();
    data->numTx = numTx + 1;
    
#ifdef DEBUG
    printSensorData(data); //To compare and demonstrate successful transmission
#endif
    
}
/*
 * For Debug Purposes, prints retrieved data
 */
void printSensorData(sensor_data_t *data)
{
    TERM_sendStringRaw("\n\nCurrent Sensor Data\n");
    TERM_sendStringRaw("-----------------------------------------------------------------------\n");
    
    printf("Relative Humidity: %u%% \r\n", data->humid);
    printf("Moisture: %u%% \r\n", data->moist);
    printf("Pressure: %u hPa\r\n", data->press);
    printf("Air Temperature: %i C \r\n", data->temp_air);
    printf("Battery Level: %u%% \r\n\n", data->battery);
    printf("\nJSON Format\r\n");
    printf("-----------------------------------------------------------------------\n");
    printf("Payload: { air_temp: %i, battery: %u, humidity: %u, moisture: %u, pressure: %u, numTx: %u\r\n\n}", 
            data->temp_air,
            data->battery,
            data->humid, 
            data->moist, 
            data->press,
            data->numTx
            );
}

/**
 * Converts Measurements to a Hex String for LoRa Transmission
 * Temperature, Humidity and Moisture only require 2 hex chars, pressure 
 * requires 4 chars, RN2903 payload string is terminated with "\r\n\0"
 */ 
void formatPayload(char *str, sensor_data_t *data) {
        
    char hex[]= "0123456789ABCDEF";
    
    str[0] = hex[((data->temp_air >> 4) & 0x0F)];
    str[1] = hex[data->temp_air & 0x0F];
    str[2] = hex[((data->humid >> 4) & 0x0F)];
    str[3] = hex[data->humid & 0x0F];
    str[4] = hex[((data->moist >> 4) & 0x0F)];
    str[5] = hex[data->moist & 0x0F];
    str[6] = hex[((data->press >> 12) & 0x0F)];
    str[7] = hex[((data->press >> 8) & 0x0F)];
    str[8] = hex[((data->press >> 4) & 0x0F)];
    str[9] = hex[data->press & 0x0F];
    str[10] = hex[((data->battery >> 4) & 0x0F)];
    str[11] = hex[data->battery & 0x0F];
    str[12] = hex[((data->numTx >> 4) & 0x0F)];
    str[13] = hex[data->numTx & 0x0F];
    str[14] = '\r';
    str[15] = '\n';
    str[16] = '\0';
}
void sendAndReceiveBuffers() {
        if(TERM__IsRxReady()) {
            LR2_tx_buff_Push(TERM_Read());
        }    
        if(LR2__IsRxReady()) {
            TERM_tx_buff_Push(LR2_Read());
        }
        if(TERM_tx_buff_Count() && TERM_IsTxReady()) {
            TERM_Write(TERM_tx_buff_Pop());
        }
        if(LR2_tx_buff_Count() && LR2_IsTxReady()) {
            LR2_Write(LR2_tx_buff_Pop());
        }
}

void BUTTON_releaseCallback(void)
{
    PORTB.OUTSET |= PIN7_bm;
    
    event_flags |= UI_BUTTON_FLAG;
}

void BUTTON_pressCallback(void)
{
    PORTB.OUTSET &= ~PIN7_bm;   
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
                
            case TTN_JOIN_REQUEST:
                state = SEND_DATA;
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
            
            sendAndReceiveBuffers();
            //TODO: Handle Join Errors
//            if(Buffer_find("accepted\r\n")) {
//                TERM_sendStringRaw("Rx: accepted\n");
//            }
            //For now automatically continues without checking for join errors
            
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
            // Put the RN2903 to sleep to save power for 120 sec
//            if(secs == 30)   {
//                LR2_tx_buff_Push_Str("sys sleep 120000\r\n");
//            }
            //After 3 minutes, return to read and transmit next measurement
            //secs = (millis / 1000); 
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






