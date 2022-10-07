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
    
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <xc.h>
//#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include "application.h"
    

void BUTTON_releaseCallback(void);
void BUTTON_pressCallback(void);  
uint16_t getNumTx();
void stateMachine();

#ifdef	__cplusplus
}
#endif

#endif	/* STATE_H */