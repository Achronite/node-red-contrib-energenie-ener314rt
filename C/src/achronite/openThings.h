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
#define OTP_ALARM           0x21
#define OTP_DEBUG_OUTPUT    0x2D
#define OTP_IDENTIFY        0x3F
#define OTP_SOURCE_SELECTOR 0x40 // write only
#define OTP_WATER_DETECTOR  0x41
#define OTP_GLASS_BREAKAGE  0x42
#define OTP_CLOSURES        0x43
#define OTP_DOOR_BELL       0x44
#define OTP_ENERGY          0x45
#define OTP_FALL_SENSOR     0x46
#define OTP_GAS_VOLUME      0x47
#define OTP_AIR_PRESSURE    0x48
#define OTP_ILLUMINANCE     0x49
#define OTP_LEVEL           0x4C
#define OTP_RAINFALL        0x4D
#define OTP_APPARENT_POWER  0x50
#define OTP_POWER_FACTOR    0x51
#define OTP_REPORT_PERIOD   0x52
#define OTP_SMOKE_DETECTOR  0x53
#define OTP_TIME_AND_DATE   0x54
#define OTP_VIBRATION       0x56
#define OTP_WATER_VOLUME    0x57
#define OTP_WIND_SPEED      0x58
#define OTP_GAS_PRESSURE    0x61
#define OTP_BATTERY_LEVEL   0x62
#define OTP_CO_DETECTOR     0x63
#define OTP_DOOR_SENSOR     0x64
#define OTP_EMERGENCY       0x65
#define OTP_FREQUENCY       0x66
#define OTP_GAS_FLOW_RATE   0x67
#define OTP_REL_HUMIDITY    0x68
#define OTP_CURRENT         0x69
#define OTP_JOIN            0x6A
#define OTP_LIGHT_LEVEL     0x6C
#define OTP_MOTION_DETECTOR 0x6D
#define OTP_OCCUPANCY       0x6F
#define OTP_REAL_POWER      0x70
#define OTP_REACTIVE_POWER  0x71
#define OTP_ROTATION_SPEED  0x72
#define OTP_SWITCH_STATE    0x73
#define OTP_TEMPERATURE     0x74
#define OTP_VOLTAGE         0x76
#define OTP_WATER_FLOW_RATE 0x77
#define OTP_WATER_PRESSURE  0x78
#define OTP_TEST            0xAA  // Not supported by energenie

/* OpenThings Command Paramters - 0x80 added*/
#define OTCP_SWITCH_STATE    0xF3

/* OpenThings Commands */
#define OTC_SWITCH_ON  0xF3, 0x01, 0x01, 0x00
#define OTC_SWITCH_OFF 0xF3, 0x01, 0x00, 0x00


// Default keys for OpenThings encryption and decryption
#define CRYPT_PID 242
#define CRYPT_PIP 0x0100
#define OT_DEFAULT_PIP 0x01, 0x00
#define OT_DEFAULT_DEVICEID 0x00, 0x20, 0x66

// Array positions
#define OTS_MSGLEN 14       // Length with only 1 command sent
#define OTH_INDEX_PRODUCTID 2
#define OTH_INDEX_DEVICEID 5
#define OT_INDEX_R1 8
#define OT_INDEX_R1_VALUE 10 


/***** FUNCTION PROTOTYPES *****/
//extern void encodeDecimal(unsigned int iDecimal, unsigned char bits, unsigned char * encArray );
extern unsigned char openThings_switch(unsigned char iProductId, unsigned int iDeviceId, unsigned char bSwitchState, unsigned char xmits);

#endif

/***** END OF FILE *****/

