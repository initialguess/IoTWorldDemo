#include "TERM_tx_buff.h"
#include "lr2.h"
#include <stdbool.h>

static char Buffer[LR2_MAX_DATA_SIZE];
static uint8_t head = 0;
static uint8_t tail = 0;


/* Adds a char to the buffer */
void TERM_tx_buff_Push(char c) {
    if (tail + 1 != head) {
        Buffer[tail] = c;
        tail++;        
    }
}

/* Adds a string to the buffer */
void TERM_tx_buff_Push_Str(char *str) {
    uint8_t pos = 0;
    char c = str[pos];
    while(tail + 1 != head && c) {
        Buffer[tail] = c;
    	tail++;
    	pos++;
    	c = str[pos];
    }
}

/* Gets the first char in the queue, and removes it from the buffer */
char TERM_tx_buff_Pop() {
    char ret = Buffer[head];
	head++;
	return ret;
}

/* Returns the number of items in the buffer */
uint8_t TERM_tx_buff_Count() {
	return head - tail;
}

void TERM_buffer_reset(void) 
{
    tail = 0;
    head = 0;
    for(int i = 0; i < LR2_MAX_DATA_SIZE; i++) 
    {
        Buffer[i] = '\0';
    }
}

//Sends a byte of data
bool TERM_sendByte(char c)
{
    if (TERM_IsTxReady())
    {
        //Clear done bit
        USART0.STATUS |= USART_TXCIF_bm;

        //Load Byte of Data
        USART0.TXDATAL = c;
        return true;  
    }
    return false;
}

//Prints the string on the UART
void TERM_sendBufferedString(void)
{   
    if (Buffer[0] == '\0')
        return;
    
    uint8_t index = 0;
    
    //Clear USART Done Flag
    USART0.STATUS |= USART_TXCIF_bm;
    
    while ((Buffer[index] != '\0') && (index < LR2_MAX_DATA_SIZE))
    {
        while (!TERM_IsTxReady());
        
        TERM_sendByte(Buffer[index]);
        index++;
    }
    
    //Wait for completion of UART
    while (TERM_IsTxDone());
}

//Check to see if buffer matches a string
//TODO: Not currently working properly
bool Buffer_find(const char* str)
{
    if (!TERM_tx_buff_Count())
    {
        return false;
    }
    //uint8_t wIndex = tail+1;
    uint8_t rIndex = head;
    uint8_t sIndex = 0;
    
    while (TERM_tx_buff_Count())
    {
        if (Buffer[rIndex] == str[sIndex])
        {
            //Matched current character
            sIndex++;
        }
        else if (Buffer[rIndex] == str[0])
        {
            //Matches position 0
            sIndex = 1;
        }
        else
        {
            //Didn't match anything
            sIndex = 0;
        }
        
        //If the string has been fully matched, return true
        if (str[sIndex] == '\0')
        {
            return true;
        }
    }
    
    return false;
}