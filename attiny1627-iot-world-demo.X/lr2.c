/*
 * File:   weather_application.c
 * Author: C70348
 *
 * Created on October 8, 2020, 2:39 PM
 */

#include "mcc_generated_files/system/system.h"
#include "lr2.h"
#include "LR2_ringBuffer.h"
#include "TERM_ringBuffer.h"
#include <util/delay.h>

//Enable print statements for Rx and Tx
#define DEBUG

//Enable Activation by Personalization
#define ABP

//Enable Over the Air Activation
//#define OTAA

void LR2_reset(void) {
#ifdef DEBUG
    TERM_sendStringRaw("\rResetting LR2 Click\n");
    TERM_sendStringRaw("----------------------------------------------------\n");
    TERM_sendStringRaw("Firmware: ");
#endif
    
    /* Reset the RN2903 */
    LR2_RST_SetHigh();
    _delay_ms(100);         
    LR2_RST_SetLow();
    _delay_ms(300);
    LR2_RST_SetHigh();
    _delay_ms(100);         
    LR2_RTS_SetHigh(); 


    
    uint8_t done = 0;
    char data;
    
    while(!done)
    {   
        if(LR2__IsRxReady()) {
            TERM_tx_buff_Push(LR2_Read());
        }
        //if(TERM_tx_buff_Count() && TERM_IsTxReady()) {
        if(TERM_IsTxReady()) {
            data = TERM_tx_buff_Pop();
            TERM_sendByte(data);
            if(data== '\n') {
                done = 1;
            }
        } 
    }
    _delay_ms(1000);
}

void LR2_sendStringCmd(char *str)
{
    //Print Command to the Terminal for Debug Purposes
    TERM_sendStringRaw("\r\nTx: ");
    TERM_sendStringRaw(str);
    
    //Add the string to the LR2 Buffer
    LR2_tx_buff_Push_Str(str);
    
    //Transmit over the LR2 UART
    for (uint8_t i = 0; str[i] != '\0'; )
    {
        if (LR2_IsTxReady())
        {
            LR2_Write(LR2_tx_buff_Pop());
            i++;
        }
    }
    //while(!TERM_IsTxReady());
    _delay_ms(100);
}

void LR2_sendAppSKey(char *str) 
{
    //Print Command to the Terminal for Debug Purposes
    //TERM_sendStringRaw("\r\n");
    TERM_sendStringRaw("\r\nTx: mac set appskey\r\n");
    
    LR2_tx_buff_Push_Str(str);
    
    _delay_ms(100);
    
    //Transmit over the LR2 UART
    for (uint8_t i = 0; str[i] != '\0'; )
    {
        if (LR2_IsTxReady())
        {
            LR2_Write(LR2_tx_buff_Pop());
            i++;
        }
    }
    _delay_ms(100);
}

void LR2_sendNwkSKey(char *str) 
{
    //Print Command to the Terminal for Debug Purposes
    TERM_sendStringRaw("\r\nTx: mac set nwkskey\r\n");
    
    LR2_tx_buff_Push_Str(str);
    
    _delay_ms(100);
    
    //Transmit over the LR2 UART
    for (uint8_t i = 0; str[i] != '\0'; )
    {
        if (LR2_IsTxReady())
        {
            LR2_Write(LR2_tx_buff_Pop());
            i++;
        }
    }
}

void LR2_recvRsp()
{
    uint8_t done = 0;

    TERM_sendStringRaw("\rRx: ");
    while(!done)
    {   
        if(LR2__IsRxReady()) {
            TERM_tx_buff_Push(LR2_Read());
        }
        if(TERM_tx_buff_Count() && TERM_IsTxReady()) {
            char data = TERM_tx_buff_Pop();
           TERM_Write(data);
            if(data== '\n') {
                done = 1;
            }
        }   
    }
}

void LR2_config_abp()
{
    TERM_sendStringRaw("\n\nConfiguring LR2 Click For ABP\n");
    TERM_sendStringRaw("-----------------------------------------------------------------------\n");
    LR2_sendStringCmd("mac set deveui " DEVEUI "\r\n");
    LR2_recvRsp();
    LR2_sendStringCmd("mac set devaddr " DEVADDR "\r\n");
    LR2_recvRsp();
    LR2_sendAppSKey("mac set appskey " APPSKEY "\r\n");
    LR2_recvRsp();
    LR2_sendNwkSKey("mac set nwkskey " NWKSKEY "\r\n");
    LR2_recvRsp();
    LR2_sendStringCmd("mac set ar on\r\n");
    LR2_recvRsp();
    LR2_sendStringCmd("mac set rxdelay1 5000\r\n");
    LR2_recvRsp();
    LR2_sendStringCmd("mac set adr on\r\n");
    LR2_recvRsp();
    LR2_sendStringCmd("mac save\r\n");
    LR2_recvRsp();
    TERM_sendStringRaw("\n\n");
    _delay_ms(100);
}

void LR2_config_otaa()
{
    TERM_sendStringRaw("\n\nConfiguring LR2 Click for OTAA\n");
    TERM_sendStringRaw("-----------------------------------------------------------------------\n");
    LR2_sendStringCmd("mac set deveui " DEVEUI "\r\n");
    LR2_recvRsp();
    LR2_sendStringCmd("mac set appkey " APPKEY "\r\n");
    LR2_recvRsp();
    LR2_sendStringCmd("mac set appeui " APPEUI "\r\n");
    LR2_recvRsp();
    LR2_sendStringCmd("mac set ar on\r\n");
    LR2_recvRsp();
    LR2_sendStringCmd("mac set rxdelay1 5000\r\n");
    LR2_recvRsp();
    LR2_sendStringCmd("mac set adr on\r\n");
    LR2_recvRsp();
    LR2_sendStringCmd("mac save\r\n");
    LR2_recvRsp();
    LR2_sendStringCmd("mac pause\r\n");
    LR2_recvRsp();
    _delay_ms(100);
    LR2_sendStringCmd("mac resume\r\n");
    LR2_recvRsp();
    TERM_sendStringRaw("\n\n");
    _delay_ms(100);
}


void LR2_join_abp() 
{
#ifdef DEBUG
    TERM_sendStringRaw("\n\nJoining TTN\n");
    TERM_sendStringRaw("-----------------------------------------------------------------------\n");
#endif
    LR2_sendStringCmd("mac join abp\r\n");
    LR2_recvRsp();
}

void LR2_join_otaa() 
{
#ifdef DEBUG
    TERM_sendStringRaw("\n\nJoining TTN\n");
    TERM_sendStringRaw("-----------------------------------------------------------------------\n");
#endif
    LR2_sendStringCmd("mac join otaa\r\n");
    LR2_recvRsp();
    
    _delay_ms(1000);
}

void LR2_config() 
{
    //Prepare LR2 for Tx/Rx according to activation procedure (ABP or OTAA))
#ifdef ABP
    //Set DEVEUI, DEVADDR, Session Keys, Auto Reply and Receive Window
    LR2_config_abp();
#endif

#ifdef OTAA
    //Set DEVEUI,DEVADDR, APPKEY, APPEUI, Auto Reply and Receive Window (5 sec)
    LR2_config_otaa();
#endif
}

void LR2_join()
{
#ifdef ABP
    //Join TTN using Activation By Personalization
    LR2_join_abp();
#endif

#ifdef OTAA
    //Join TTN using Over the Air Activation
    LR2_join_otaa();
#endif
}

//Transmits a formatted payload string on port 1 using confirmed uplink
void LR2_tx_cnf(char *str) {
#ifdef DEBUG
    TERM_sendStringRaw("\n\nTransmitting Data\n");
    TERM_sendStringRaw("----------------------------------------------------\n");
    //Prints Command and Payload on Terminal for debugging
    TERM_sendStringRaw("\nTx: mac tx cnf 42 ");
    TERM_sendStringRaw(str);
#endif
    
    //Send Transmission to LR2 UART Tx
    LR2_tx_buff_Push_Str("mac tx cnf 42 ");
    LR2_tx_buff_Push_Str(str);    
}

//Transmits a formatted payload string on port 1 using unconfirmed uplink
void LR2_tx_uncnf(char *str) {
#ifdef DEBUG
    TERM_sendStringRaw("\n\nTransmitting Data\n");
    TERM_sendStringRaw("----------------------------------------------------\n");
    //Prints Command and Payload on Terminal for debugging
    TERM_sendStringRaw("\nTx: mac tx uncnf 43 ");
    TERM_sendStringRaw(str);
#endif
    
    //Send Transmission to LR2 UART Tx
    LR2_tx_buff_Push_Str("mac tx uncnf 43 ");
    LR2_tx_buff_Push_Str(str);
}
