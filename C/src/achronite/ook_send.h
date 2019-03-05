/* ook_send.h  Achronite, January 2019
 * 
 * Simplified interface for ENER314-RT devices using OOK protocol on Raspberry Pi
 */

#ifndef OOKSEND_H
#define OOKSEND_H

#include <stdlib.h>

// preamble pulse with timing violation gap
#define PREAMBLE 0x80, 0x00, 0x00, 0x00

// Energenie deafult 20 bit address is 0x6C6C6
// 0110 1100 0110 1100 0110
// 0 encoded as 8 (1000)
// 1 encoded as E (1110)
#define ZONE_BITS 20
#define DEFAULT_HC 0x8E, 0xE8, 0xEE, 0x88, 0x8E, 0xE8, 0xEE, 0x88, 0x8E, 0xE8
#define USE_DEFAULT_ZONE 0

// Array positions
#define OOK_MSGLEN 16
#define INDEX_HC 4
#define INDEX_SC 14

/***** FUNCTION PROTOTYPES *****/
extern void encodeDecimal(unsigned int iDecimal, unsigned char bits, unsigned char * encArray );
extern unsigned char OokSend(unsigned int iZone, unsigned char iSwitchNum, unsigned char bSwitchState, unsigned char xmits);


#endif

/***** END OF FILE *****/

