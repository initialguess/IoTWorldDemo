/* 
 * File:   state.h
 *
 * Created on July 17, 2022, 2:41 PM
 */

#ifndef STATE_H
#define	STATE_H

#ifdef	__cplusplus
extern "C" {
#endif
    

#include <xc.h>
#include "mcc_generated_files/system/system.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include "bme280.h"
#include "TWI.h"    
#include "lr2.h"
#include "LR2_ringBuffer.h"
#include "TERM_ringBuffer.h"
#include "state.h"
    
#define CONVERSION_PERCENT 0.0625
#define IN_MAX 2791 //3064          
#define IN_MIN 1191 //1190
    
//Enable print statements for Rx and Tx
#define DEBUG
    
//Enable Activation by Personalization
#define ABP

//Enable Over the Air Activation
//#define OTAA
    
typedef struct
{
    int8_t temp_air;     // Air Temperature    Weather Click
    uint8_t humid;       // Humidity           Weather Click
    uint16_t press;      // Pressure           Weather Click
    uint8_t moist;       // Soil Moisture      ADCC
    uint8_t battery;     // Battery Level
    uint16_t numTx;       // LoRa Tx Number     

} sensor_data_t;

void WeatherClick_initialize(void);
void PrintUtility_enable(void);
void WeatherClick_readSensors(void);
void printRawValues(void);
uint8_t map(uint16_t raw);
uint8_t lookup(uint16_t raw);
void Soil_Pulse(void);
uint8_t getMoistureMeasurement();
uint8_t getSoilTemp();
uint8_t getBatteryLevel();
void getSensorData(sensor_data_t *);
void printSensorData(sensor_data_t *);
void formatPayload(char *, sensor_data_t *);
void sendAndReceiveBuffers();
void BUTTON_releaseCallback(void);
void BUTTON_pressCallback(void);  
uint16_t getNumTx();
void stateMachine();

#ifdef	__cplusplus
}
#endif

#endif	/* STATE_H */