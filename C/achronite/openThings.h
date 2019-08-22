/* openThings.h  Achronite, March 2019
 * 
 * Simplified interface for ENER314-RT devices using OpenThings protocol on Raspberry Pi
 */

#ifndef OTSEND_H
#define OTSEND_H

#include <stdlib.h>

#define FSK_MODE 1
#define ENERGENIE_MFRID 0x04

/* OpenThings Product IDs */
#define PRODUCTID_MIHO004 0x01   // Monitor only
#define PRODUCTID_MIHO005 0x02   // Adaptor Plus
#define PRODUCTID_MIHO006 0x05   // House Monitor
#define PRODUCTID_MIHO013 0x03   // eTRV
#define PRODUCTID_MIHO032 0x0C   // FSK motion sensor
#define PRODUCTID_MIHO033 0x0D   // FSK open sensor

/* OpenThings Parameter Keys (for read)
** To WRITE/Command any of these add 128 (0x80) to set bit 7
*/
struct OT_PARAM {
  char paramName[15];
  char paramId;
};
#define NUM_OT_PARAMS 46


/* OpenThings Command Paramters - 0x80 added*/
#define OTCP_SWITCH_STATE    0xF3
#define OTCP_JOIN            0xEA

// OpenThings record data types
#define	OT_UINT   0x00
#define	OT_UINT4  0x10    // 4
#define	OT_UINT8  0x20    // 8
#define	OT_UINT12 0x30    // 12
#define	OT_UINT16 0x40    // 16
#define	OT_UINT20 0x50    // 20
#define	OT_UINT24 0x60    // 24
#define	OT_CHAR   0x70
#define	OT_SINT   0x80    // dec=128
#define	OT_SINT8  0x90    // 8
#define	OT_SINT16 0xA0    // 16
#define	OT_SINT24 0xB0    // 24
#define	OT_FLOAT  0xF0    // Not implemented yet

/* OpenThings Commands (as we don't support auto build of these yet) */
// PARAM, TYPEID, VALUE BYTES
#define OTC_SWITCH_ON  0xF3, 0x01, 0x01, 0x00       // Switch ON
#define OTC_SWITCH_OFF 0xF3, 0x01, 0x00, 0x00       // Switch OFF
#define OTC_JOIN_ACK   0x6A, 0x00, 0x00             // Join ACK


// Default keys for OpenThings encryption and decryption
#define CRYPT_PID 242
#define CRYPT_PIP 0x0100
#define OT_DEFAULT_PIP 0x01, 0x00
#define OT_DEFAULT_DEVICEID 0x00, 0x20, 0x66

#define OT_MAX_RECS 0xF

// Array positions
#define OTS_MSGLEN 14       // Switch command - Length with only 1 command sent
#define OTA_MSGLEN 13       // ACK command - Length with 1 command without value!
#define OTH_INDEX_PRODUCTID 2
#define OTH_INDEX_DEVICEID 5
#define OT_INDEX_R1 8
#define OT_INDEX_R1_VALUE 10 

// OpenThings record
struct OTrecord {
    unsigned char wr;
    unsigned char paramId;
    char paramName[15];
    unsigned char typeId;
    char  typeIndex;
    int   retInt;                // I'm hoping this deals with signed and unsigned values
    float retFloat;
    char  retChar[15];            // Length max is 15 for a record
};

#define OTR_INT 1
#define OTR_FLOAT 2
#define OTR_CHAR 3

struct OT_DEVICE {
    unsigned int  deviceId;
    unsigned char mfrId;
    unsigned char productId;
    bool          control;
    bool          joined;
    char          product[14];
};

#define MAX_DEVICES 30


struct OT_PRODUCT {
    unsigned char mfrId;
    char productId;
    bool control;
    char product[15];
};
#define NUM_OT_PRODUCTS 8


/***** FUNCTION PROTOTYPES *****/
unsigned char openThings_switch(unsigned char iProductId, unsigned int iDeviceId, unsigned char bSwitchState, unsigned char xmits);
unsigned char openThings_deviceList(char *devices, bool scan);
int openThings_receive(char *OTmsg );

unsigned char openThings_joinACK(unsigned char iProductId, unsigned int iDeviceId, unsigned char xmits);
void openthings_scan(int iTimeOut);

#endif

/***** END OF FILE *****/

