#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "openThings.h"
#include "../energenie/radio.h"
#include "../energenie/trace.h"

/*
** C module addition to energenie code to simplify the FSK OpenThings interaction with the Energenie ENER314-RT
** by minimising the number of calls required to interact with C radio device.
**
** Author: Phil Grainger - @Achronite, March 2019
*/

// Globals - yuck
unsigned short ran;

/*
** calculateCRC()- Calculate an OpenThings CRC
** Code converted from python module by @whaleygeek
*/
unsigned short calculateCRC(unsigned char *msg, unsigned int length)
{
    unsigned char ch, bit;
	unsigned short rem = 0, i;   //uint16_t

	for (i=0; i<length; i++)
    {
        ch = msg[i];
        //printf("%d=%d\n",i,ch);
		rem = rem ^ (ch<<8);
		for (bit=0; bit<8; bit++){
			if ((rem & (1<<15)) != 0){
				// bit is set
				rem = ((rem<<1) ^ 0x1021);
            } else {
				// bit is clear
				rem = (rem<<1);
            }
        }
    }
	return rem;
}

/*
** encryptByte()- encrypt a single Byte of an OpenThings message
** Code converted from python module by @whaleygeek
*/
unsigned char encryptByte(unsigned char data)
{
    unsigned char i;

    for (i=0; i<5; i++){
        if ((ran&0x01) != 0){
            // bit0 set
            ran = ((ran>>1) ^ 62965);
        }else{
            // bit0 clear
            ran = ran >> 1;
        }
    }
    return (ran ^ data ^ 90);
}

/*
** encryptMsg() - encrypt an OpenThings message (destructive)
**
** Code converted from python module by @whaleygeek
*/
void encryptMsg(unsigned char pid, unsigned short pip, unsigned char *msg, unsigned int length)
{
    unsigned char i;

    ran = (((pid&0xFF)<<8) ^ pip);
    for (i=0; i<length; i++){
        msg[i] = encryptByte(msg[i]);
    }
}

/*
** openThings_switch()
** =======
** Send a switch signal to a 'Control and Monitor' RF FSK OpenThings based Energenie smart device
** Currently this covers the 'HiHome Adaptor Plus' and 'MiHome Heating' TRV
**
** The OpenThings messages are comprised of 3 parts:
**  Header  - msgLength, manufacturerId, productId, encryptionPIP, and deviceId
**  Records - The body of the message, in this case a single command to switch the state
**  Footer  - CRC
**
** Functions performed include:
**    initialising the radio and setting the modulation
**    encoding of the device and switch status
**    formatting and encoding the OpenThings FSK radio request
**    sending the radio request via the ENER314-RT RaspberryPi adaptor
*/
unsigned char openThings_switch(unsigned char iProductId, unsigned int iDeviceId, unsigned char bSwitchState, unsigned char xmits)
{
    int ret = 0;
    unsigned short crc;
/*
	payload.append(0) # length, fixup later when known
	payload.append(header["mfrid"])
	payload.append(header["productid"])
	payload.append((encryptPIP&0xFF00)>>8) # MSB
	payload.append((encryptPIP&0xFF))      # LSB
	payload.append((sensorId>>16) & 0xFF) # HIGH
	payload.append((sensorId>>8) & 0xFF)  # MID
	payload.append((sensorId) & 0XFF)     # LOW

socket 0 OFF
TYPE:LE,MF,PR, PIP ,  DeviceId   , CMD,UINT,VAL, NUL, CRC, CRC
un: [13, 4, 2, 1, 0,   0,  32, 102, 243,   1,  0,   0, 210, 240]
enc:[13, 4, 2, 1, 0, 194, 188, 161,  12, 245, 67, 241, 210,  59]
socket 0 ON
un: [13, 4, 2, 1, 0,   0,  32, 102, 243,   1,  1,   0, 225, 193]
enc:[13, 4, 2, 1, 0, 194, 188, 161,  12, 245, 66, 241, 225,  10]

*/
    unsigned char radio_msg[OTS_MSGLEN] = {OTS_MSGLEN-1, ENERGENIE_MFRID, PRODUCTID_MIHO005, OT_DEFAULT_PIP, OT_DEFAULT_DEVICEID, OTC_SWITCH_OFF, 0x00, 0x00};

    printf("openThings_switch: productId=%d, deviceId=%d, state=%d\n",iProductId, iDeviceId,bSwitchState);

    /*
    ** OpenThings HEADER
    */
    // productId (usually 2 for MIHO005)
    radio_msg[OTH_INDEX_PRODUCTID] = iProductId;

    /*
    ** OpenThings RECORDS (Commands)
    */
    // deviceId
    radio_msg[OTH_INDEX_DEVICEID]   = (iDeviceId>>16) & 0xFF; //MSB
    radio_msg[OTH_INDEX_DEVICEID+1] = (iDeviceId>>8) & 0xFF;  //MID
    radio_msg[OTH_INDEX_DEVICEID+2] = iDeviceId & 0xFF;       //LSB

    if (bSwitchState){
        // We already have the switch off command in the message, just override the switch value to on
        radio_msg[OT_INDEX_R1_VALUE] = 1;
    }

    /*
    ** OpenThings FOOTER (CRC)
    */
    crc = calculateCRC(&radio_msg[5], (OTS_MSGLEN-7));
    radio_msg[OTS_MSGLEN-2] = ((crc>>8) & 0xFF);  // MSB
	radio_msg[OTS_MSGLEN-1] = (crc&0xFF); // LSB

/*
    printf("openThings_switch: unencrypted msg: [");
    for (ret=0; ret<OTS_MSGLEN;ret++){
        printf("%3d,",radio_msg[ret]);
    }
    printf("] crc=%d\n",crc);
*/

    // encrypt body of message
    encryptMsg(CRYPT_PID, CRYPT_PIP, &radio_msg[5], (OTS_MSGLEN-5));

    /*
    ** Transmit via radio adaptor
    */

    // Set FSK mode for OpenThings devices
    radio_modulation(RADIO_MODULATION_FSK);

    // Transmit encoded payload 26ms per payload * xmits
    radio_transmit(radio_msg,OTS_MSGLEN,xmits);

    // place radio into standby mode, this may need to change to support receive mode
    radio_standby();
  
    return ret;
}