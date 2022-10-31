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
#define RN2903_CONFIGURED           (1 << 1)
#define TTN_JOIN_SUCCESS_FLAG       (1 << 2)
#define TTN_JOIN_ERROR_FLAG         (1 << 3)
#define TX_SUCCESS_FLAG             (1 << 4)
#define TX_ERROR_FLAG               (1 << 5)
#define SLEEP_TIMER_FLAG            (1 << 6)

// defines for Soil Moisure LUT
#define LUT_TABLESIZE 42    // number of elements in lookup table (in pairs of ADC read and %)
#define LUT_MIN 2019        // corresponds to 100% soil moisture
#define LUT_MAX 3400        // corresponds to 0% soil moisture
#define LUT_STEPSIZE 5      // steps between soil moisture percentage

typedef enum    {
    TTN_NOT_JOINED,
    TTN_JOIN_REQUEST,
    TTN_JOINED,
    SEND_DATA,
    SLEEP,
    WAIT_FOR_BUTTON,        
    TESTS
} STATE;

// For now in Test Phase, change to TTN_NOT_JOINED for normal operation
// After each test, State returns to WAIT_FOR_BUTTON, then advances to next TEST on a button press
static STATE state = TTN_NOT_JOINED;
uint16_t numTest = 0;

bool weather_initialized = 0;
bool label_initial = false;
static volatile uint16_t event_flags = 0;
volatile uint32_t millis = 0;   // counter for elapsed milliseconds
uint32_t last_millis = 0;
uint32_t secs = 0;              // counter for elapsed seconds
uint32_t soilSecs = 0;
uint32_t lastsecs = 0;          // reference to determine total elapsed time in sec
uint16_t numTx = 0;

//LR2 payload for TTN - String of hex characters representing sensor data
char payload[19];

//Data structure: measurements from Weather Click and moisture sensor
static sensor_data_t data;

// static because we dont want to allocate and initialize on the stack every time the associated function is called
// const because we dont ever want to write to the table, this will catch accidental writes to the table
// __flash because we dont want to copy the table into ram every time we use it, we only have 3KB SRAM
// Lookup Table for Soil Moisture
// keep table 
static const __flash uint16_t lut[LUT_TABLESIZE] = {
    2019, 100,
    2020, 95,
    2029, 90,
    2044, 85,
    2067, 80,
    2097, 75,
    2134, 70, 
    2178, 65,
    2230, 60,
    2288, 55,
    2354, 50,
    2426, 45,
    2506, 40,
    2593, 35,
    2687, 30,
    2788, 25,
    2896, 20,
    3011, 15,
    3134, 10,
    3263, 5,
    3400, 0
};


// 1ms interrupt
ISR(TCB0_INT_vect)  {
    millis++;
    TCB0.INTFLAGS |= TCB_CAPT_bm;
}


ISR(TCB1_INT_vect)
{
    if (PORTC.IN & PIN1_bm)
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

// pulse soil moisture sensor for 1 sec at ~868kHz
void Soil_Pulse(void)   {    
    static uint32_t loop_cnt = 0;
    loop_cnt++;
    soilSecs = ((millis - last_millis) / 1000UL); 
    if(soilSecs - lastsecs >= 2UL) {
        last_millis = millis;    
    }
    
}
uint8_t lookup(uint16_t raw) {
    
    // catch any ADC reads outside of specified bounds
    if (raw < LUT_MIN) return 100;  // 100% soil moisture
    else if (raw > LUT_MAX) return 0;   // 0% soil moisture
    
    else    {
        uint8_t i = 0;
        // iterate through LUT ADC values until we find entry larger than our raw ADC read
        while (lut[i] < raw)    {
            i+=2;
        }
        // calculate the scale (or percentage) of how big the ADC value is compared to the steps between LUT values
        // the 1.0 is because we are counting ADC values UP as we go through the lookup table, but DOWN as we go though soil moisture %
        float scale = 1.0 - ((float)(raw - lut[i-2]) / (float)(lut[i] - lut[i-2]));
        // calculate percent based on how far our ADC read is between LUT steps and the lower bound of the soil moisture percentage
        float percent = lut[i+1] + (scale * LUT_STEPSIZE);
        // convert to int for simplicity
        uint8_t soil_moist_percent = (int)round(percent);
        //printf("Soil Moisture Int: %d%%\n", soil_moist_percent);
        return soil_moist_percent;
    }
    
}

uint8_t getMoistureMeasurement() {
    Soil_Pulse();
    uint16_t val = ADC0_GetConversion(7);
    printf("%u\r\n", val);
    return lookup(val);    
}

uint8_t getSoilTemp()   {
    uint8_t temp = BME280_getTemperature() - 5;
    return temp;
}

uint8_t getBatteryLevel()   {
    //For Testing Purposes loses 1% every 1000 Tx's
    uint16_t level = 100 - (numTx / 1000); 
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
    str[12] = hex[((data->numTx >> 12) & 0x0F)];
    str[13] = hex[((data->numTx >> 8) & 0x0F)];
    str[14] = hex[((data->numTx >> 4) & 0x0F)];
    str[15] = hex[data->numTx & 0x0F];
    str[16] = '\r';
    str[17] = '\n';
    str[18] = '\0';
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
    //PORTC.OUTSET |= PIN0_bm;
    
    event_flags |= UI_BUTTON_FLAG;
}

void BUTTON_pressCallback(void)
{
    //PORTC.OUTSET &= ~PIN0_bm;   
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
                event_flags |= TTN_JOIN_SUCCESS_FLAG;
                
                state = TTN_JOIN_REQUEST; 
                 
                break;
                
            case TTN_JOIN_REQUEST:
                state = SEND_DATA;
                LED0_SetHigh();
                break;
                
            case TTN_JOINED:
                state = SEND_DATA;
                break;
                
            case WAIT_FOR_BUTTON:
                state = TESTS;
                numTest++;
                break;
                
               
            default:
                break;
        }
        event_flags &= ~UI_BUTTON_FLAG;
    }
    
    //Toggle LED when RN2903 has successfully joined 
    if(event_flags & TTN_JOIN_SUCCESS_FLAG) {
        LED0_SetLow();
        event_flags &= ~TTN_JOIN_SUCCESS_FLAG;
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
            //Turn on LED to signal SEND DATA start
            //LED0_SetHigh();
            //Gather and format sensor data for transmission
            getSensorData(&data);
            formatPayload(payload, &data);
            
            //LR2_RTS_SetHigh(); 
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
//            if(secs == 2) {
//                LED0_SetLow();
//            }
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
            
        case WAIT_FOR_BUTTON:
            break;
            
        case TESTS:
            switch(numTest) {
                case 1:
                    printf("Begin Test %d\r\n", numTest);
                    state = WAIT_FOR_BUTTON;
                    _delay_ms(100);
                    break;
                case 2:
                    printf("Test %d BME280", numTest);
                    getSensorData(&data);
                    state = WAIT_FOR_BUTTON;
                    break;
                case 3:
                    printf("Test %d LoRa", numTest);
                    /* Reset the RN2903 */
                    LR2_RST_SetHigh();
                    _delay_ms(100);         
                    LR2_RST_SetLow();
                    _delay_ms(300);
                    LR2_RST_SetHigh();
                    _delay_ms(100);         
                    LR2_RTS_SetHigh(); 
                    _delay_ms(1000);
                    state = WAIT_FOR_BUTTON;
                    break;
                default:
                    break;
            }
        default:
            break;
    }

}
