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
// OpenThings FSK paramters (known)  [{ParamName, paramId}]
// I've moved the likely ones to the top for speed
static struct OT_PARAM OTparams[NUM_OT_PARAMS] = {
    {"UNKNOWN",         0x00},
    {"FREQUENCY",       0x66},
    {"REAL_POWER",      0x70},
    {"REACTIVE_POWER",  0x71},
    {"VOLTAGE",         0x76},
    {"ALARM",           0x21},
    {"DEBUG_OUTPUT",    0x2D},
    {"IDENTIFY",        0x3F},
    {"SOURCE_SELECTOR", 0x40}, // write only
    {"WATER_DETECTOR",  0x41},
    {"GLASS_BREAKAGE",  0x42},
    {"CLOSURES",        0x43},
    {"DOOR_BELL",       0x44},
    {"ENERGY",          0x45},
    {"FALL_SENSOR",     0x46},
    {"GAS_VOLUME",      0x47},
    {"AIR_PRESSURE",    0x48},
    {"ILLUMINANCE",     0x49},
    {"LEVEL",           0x4C},
    {"RAINFALL",        0x4D},
    {"APPARENT_POWER",  0x50},
    {"POWER_FACTOR",    0x51},
    {"REPORT_PERIOD",   0x52},
    {"SMOKE_DETECTOR",  0x53},
    {"TIME_AND_DATE",   0x54},
    {"VIBRATION",       0x56},
    {"WATER_VOLUME",    0x57},
    {"WIND_SPEED",      0x58},
    {"GAS_PRESSURE",    0x61},
    {"BATTERY_LEVEL",   0x62},
    {"CO_DETECTOR",     0x63},
    {"DOOR_SENSOR",     0x64},
    {"EMERGENCY",       0x65},
    {"GAS_FLOW_RATE",   0x67},
    {"REL_HUMIDITY",    0x68},
    {"CURRENT",         0x69},
    {"JOIN",            0x6A},
    {"LIGHT_LEVEL",     0x6C},
    {"MOTION_DETECTOR", 0x6D},
    {"OCCUPANCY",       0x6F},
    {"ROTATION_SPEED",  0x72},
    {"SWITCH_STATE",    0x73},
    {"TEMPERATURE",     0x74},
    {"WATER_FLOW_RATE", 0x77},
    {"WATER_PRESSURE",  0x78},
    {"TEST",            0xAA}
};

/* OpenThings Command Paramters - 0x80 added*/
#define OTCP_SWITCH_STATE    0xF3

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

/* OpenThings Commands */
#define OTC_SWITCH_ON  0xF3, 0x01, 0x01, 0x00
#define OTC_SWITCH_OFF 0xF3, 0x01, 0x00, 0x00


// Default keys for OpenThings encryption and decryption
#define CRYPT_PID 242
#define CRYPT_PIP 0x0100
#define OT_DEFAULT_PIP 0x01, 0x00
#define OT_DEFAULT_DEVICEID 0x00, 0x20, 0x66

#define OT_MAX_RECS 0xF

// Array positions
#define OTS_MSGLEN 14       // Length with only 1 command sent
#define OTH_INDEX_PRODUCTID 2
#define OTH_INDEX_DEVICEID 5
#define OT_INDEX_R1 8
#define OT_INDEX_R1_VALUE 10 

// Radio constants (from radio.c)
#define MAX_FIFO_BUFFER   66

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
    char          product[14];
};

#define MAX_DEVICES 30


struct OT_PRODUCT {
    unsigned char mfrId;
    char productId;
    bool control;
    char product[14];
};
// OpenThings FSK products (known)  [{mfrId, productId, control (boolean), product}]
#define NUM_OT_PRODUCTS 8
static struct OT_PRODUCT OTproducts[NUM_OT_PRODUCTS] = {
    {4, 0x00, true,  "Unknown"      },
    {4, 0x01, false, "Monitor Plug" },
    {4, 0x02, true,  "Adaptor Plus" },
    {4, 0x05, false, "House Monitor"},
    {4, 0x03, true,  "eTRV"         },
    {4, 0x0C, false, "Motion Sensor"},
    {4, 0x0D, false, "Open Sensor"  },
    {4, 0x00, true,  "Thermostat",  }   // I dont know the productId of this yet
};

// Rx Message
struct RADIO_MSG {
    time_t t;
    unsigned char msg[MAX_FIFO_BUFFER];
};
#define RX_MSGS 5

/***** FUNCTION PROTOTYPES *****/
//extern void encodeDecimal(unsigned int iDecimal, unsigned char bits, unsigned char * encArray );
extern unsigned char openThings_switch(unsigned char iProductId, unsigned int iDeviceId, unsigned char bSwitchState, unsigned char xmits);
extern unsigned char openThings_deviceList(unsigned char iTimeOut, char *devices );
extern char openThings_receive(unsigned char iTimeOut, char *OTmsg );
int empty_radio_Rx_buffer();
//unsigned char openThings_learn(unsigned char iTimeOut, char *devices)

#endif

/***** END OF FILE *****/

