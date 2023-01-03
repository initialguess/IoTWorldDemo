/*
  (c) 2020 Microchip Technology Inc. and its subsidiaries.

  Subject to your compliance with these terms, you may use Microchip software
  and any derivatives exclusively with Microchip products. You're responsible
  for complying with 3rd party license terms applicable to your use of 3rd
  party software (including open source software) that may accompany Microchip
  software.

  SOFTWARE IS "AS IS." NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY,
  APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT,
  MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.

  IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
  INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
  WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP
  HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO
  THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL
  CLAIMS RELATED TO THE SOFTWARE WILL NOT EXCEED AMOUNT OF FEES, IF ANY,
  YOU PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
*/

#ifndef LR2_H
#define	LR2_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "mcc_generated_files/system/system.h"
    
/* Device EUI and Address */    
#define DEVEUI "70B3D57ED0058E3D"
#define DEVADDR "260C6636"
    
/* ABP Session Keys */
#define NWKSKEY "EB0B6CE0489E9A679AB2173CBD44FCA3"
#define APPSKEY "8C00B40CCB3276A4C77E21E37146A845"
   
/* OTAA Session Keys */
#define APPEUI "0000000000000000"
#define APPKEY "8C027FFA452D40B75882EDA8BC0AF5E7"    
 
/* LR2 Click Commands */
#define LR2_CMD_SYS_GET_VER          "sys get ver"
#define LR2_CMD_SYS_RESET            "sys reset"    
#define LR2_CMD_MAC_PAUSE            "mac pause"
#define LR2_CMD_RADIO_SET_WDT        "radio set wdt 0"
#define LR2_ARG_0                    "0"

/* LR2 Join Procedure Type Mode */
#define LR2_JM_OTAA                  "otaa"
#define LR2_JM_ABP                   "abp"

/* LR2 Uplink Payload Type */
#define LR2_PL_CNF                   "cnf "
#define LR2_PL_UNCNF                 "uncnf "

/* LR2 Command and Response Strings Maximum Size */
#define LR2_MAX_DATA_SIZE            256
#define LR2_MAX_TRANSFER_SIZE        384
#define LR2_MAX_CMD_SIZE             64 
#define LR2_MAX_RSP_SIZE             20    
    
/* LR@ Number of Response Indexes */
#define LR2_RESP_COUNT               100

/* LR2 Timer Limit */
#define LR2_TIMER_EXPIRED            50000
    
/* LR2 Mac and Radio Join and Transmit Commands */
#define LR2_MAC_TX                   "mac tx "
#define LR2_JOIN                     "mac join "
#define LR2_RADIO_TX                 "radio tx "
#define LR2_RADIO_RX                 "radio rx "

/* LR2 RX Buffer Size */
#define RX_BUFFER_SIZE               300
    
void LR2_reset(void);
void LR2_sendStringCmd(char *);
void LR2_sendAppSKey(char *);
void LR2_sendNwkSKey(char *);
void LR2_recvRsp(void);
void LR2_config_abp(void);
void LR2_config_otaa(void);
void LR2_join_abp(void);
void LR2_join_otaa(void);
void LR2_config(void);
void LR2_join(void);    
void LR2_tx_cnf(char *);
void LR2_tx_uncnf(char *);


#ifdef	__cplusplus
}
#endif

#endif	/* LR2_H */



