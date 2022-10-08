/* 
 * File:   TERM_tx_buff.h
 * 
 *
 * Created on March 11, 2022, 2:53 PM
 */

#ifndef TERM_ringBuffer_H
	#define TERM_ringBuffer_H

    #include "mcc_generated_files/system/system.h"
	
	/*Function prototypes*/
    void TERM_tx_buff_Push(char c);
    void TERM_tx_buff_Push_Str(char *str);
    char TERM_tx_buff_Pop();
    uint8_t TERM_tx_buff_Count();
    void TERM_buffer_reset(void);
    bool TERM_sendByte(char c);
    void TERM_sendBufferedString(void);
    void TERM_sendStringRaw(const char* text);
    bool Buffer_find(const char*);
#endif

