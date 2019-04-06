#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include "openThings.h"
#include "lock_radio.h"
#include "../energenie/radio.h"
#include "../energenie/hrfm69.h"
#include "../energenie/trace.h"

/*
** C module addition to energenie code to simplify the FSK OpenThings interaction with the Energenie ENER314-RT
** by minimising the number of calls required to interact with C radio device.
**
** Author: Phil Grainger - @Achronite, March 2019
*/

// Globals - yuck
unsigned short ran;
struct OT_DEVICE OTdevices[MAX_DEVICES]; // should maybe make this dynamic!
static int NumDevices = 0;
static struct RADIO_MSG RxMsgs[RX_MSGS];
static int pRxMsgHead = 0; // pointer to the next slot to load a Rx msg into in our Rx Msg FIFO
static int pRxMsgTail = 0; // pointer to the next msg to read from Rx Msg FIFO, if head=tail the buffer is empty

/*
** calculateCRC()- Calculate an OpenThings CRC
** Code converted from python module by @whaleygeek
*/
unsigned short calculateCRC(unsigned char *msg, unsigned int length)
{
    unsigned char ch, bit;
    unsigned short rem = 0, i; //uint16_t

    for (i = 0; i < length; i++)
    {
        ch = msg[i];
        //printf("%d=%d\n",i,ch);
        rem = rem ^ (ch << 8);
        for (bit = 0; bit < 8; bit++)
        {
            if ((rem & (1 << 15)) != 0)
            {
                // bit is set
                rem = ((rem << 1) ^ 0x1021);
            }
            else
            {
                // bit is clear
                rem = (rem << 1);
            }
        }
    }
    return rem;
}

/*
** cryptByte()- en/decrypt a single Byte of an OpenThings message
** Code converted from python module by @whaleygeek
*/
unsigned char cryptByte(unsigned char data)
{
    unsigned char i;

    for (i = 0; i < 5; i++)
    {
        if ((ran & 0x01) != 0)
        {
            // bit0 set
            ran = ((ran >> 1) ^ 62965);
        }
        else
        {
            // bit0 clear
            ran = ran >> 1;
        }
    }
    return (ran ^ data ^ 90);
}

/*
** cryptMsg() - en/decrypt an OpenThings message (destructive)
**
** Code converted from python module by @whaleygeek
*/
void cryptMsg(unsigned char pid, unsigned short pip, unsigned char *msg, unsigned int length)
{
    unsigned char i;

    ran = (((pid & 0xFF) << 8) ^ pip);
    for (i = 0; i < length; i++)
    {
        msg[i] = cryptByte(msg[i]);
    }
}

/*
** OTtypelen() - return the number of bits used to encode a specific OpenThings record data type
*/
char OTtypelen(unsigned char OTtype)
{
    char bits = 0;

    switch (OTtype)
    {
    case OT_UINT4:
        bits = 4;
        break;
    case OT_UINT8:
    case OT_SINT8:
        bits = 8;
        break;
    case OT_UINT12:
        bits = 12;
        break;
    case OT_UINT16:
    case OT_SINT16:
        bits = 16;
        break;
    case OT_UINT20:
        bits = 20;
        break;
    case OT_UINT24:
    case OT_SINT24:
        bits = 24;
    }
    return bits;
}

/* finds the product code in the OTproducts array and returns index
*/
int openThings_getProductIndex(const char id)
{
    for (size_t i = 1; i < NUM_OT_PRODUCTS; i++)
    {
        if (OTproducts[i].productId == id)
            return i;
    }
    return 0; // unknown
}

/* finds the param code in the OTparams array and returns index
*/
int openThings_getParamIndex(const char id)
{
    for (size_t i = 1; i < NUM_OT_PARAMS; i++)
    {
        if (OTparams[i].paramId == id)
            return i;
    }
    return 0; // unknown
}

/* openThings_getDeviceIndex() - finds the id in the OTdevices array and returns index if it exists, otherwise return -1
*/
int openThings_getDeviceIndex(unsigned int id)
{
    for (int i = 0; i < NumDevices; i++)
    {
        if (OTdevices[i].deviceId == id)
            return i;
    }
    return -1;
}

/*
** openThings_devicePut() - add device to deviceList if it is not already there
*/
void openThings_devicePut(unsigned int iDeviceId, unsigned char mfrId, unsigned char productId)
{
    int OTpi;

    if (openThings_getDeviceIndex(iDeviceId) < 0)
    {
        // new device
        OTdevices[NumDevices].mfrId = mfrId;
        OTdevices[NumDevices].productId = productId;
        OTdevices[NumDevices].deviceId = iDeviceId;

        // add product characteristics
        OTpi = openThings_getProductIndex(productId);
        OTdevices[NumDevices].control = OTproducts[OTpi].control;
        strcpy(OTdevices[NumDevices].product, OTproducts[OTpi].product);

        printf("openThings_devicePut() device %d(%d) added\n", NumDevices, iDeviceId);
        NumDevices++;
    }
    // else
    // {
    //     printf("openThings_devicePut() device %d already exist\n", iDeviceId);
    // }
}

/*
** openThings_decode()
** ===================
** Decode an OpenThings payload
**
** The OpenThings messages are comprised of 3 parts:
**  Header  - msgLength, manufacturerId, productId, encryptionPIP, and deviceId
**  Records - The body of the message, in this case a single command to switch the state
**  Footer  - CRC
**
** Functions performed here include:
*/
int openThings_decode(unsigned char *payload, unsigned char *mfrId, unsigned char *productId, unsigned int *iDeviceId, struct OTrecord recs[])
{
    unsigned char length, i, j, param, rlen;
    unsigned short pip, crc, crca;
    int result = 0;
    int record = 0;
    // float f;

    //struct openThingsHeader sOTHeader;

    /* max buffer is 66, so what is reasonable for records array?  bytes left after CRC (2), Header (8) = 56
     * each record has 2 bytes header and a variable length payload of up to 15 (0xF) bytes based on type,
     * which looking at the spec is max 24bits (excluding FLOAT), so 3-17 bytes / record; so max would be 56/3=18 records
     */

    length = payload[0];

    // ignore double check on length for now, as hrfm function does not return this
    /*
    if ((length+1 != len(payload)) || (length < 10)){
        // broken
        return -1;
    } else {
    */
    // DECODE HEADER
    *mfrId = payload[1];
    *productId = payload[2];
    pip = (payload[3] << 8) + payload[4];

    //struct sOTHeader = { length, mfrId, productId, pip };

    // decode body
    cryptMsg(CRYPT_PID, pip, &payload[5], (length - 4));
    *iDeviceId = (payload[5] << 16) + (payload[6] << 8) + payload[7];

    // CHECK CRC from last 2 bytes of message
    crca = (payload[length - 1] << 8) + payload[length];
    crc = calculateCRC(&payload[5], (length - 6));

    if (crc != crca)
    {
        // CRC does not match
        printf("openThings_decode(): ERROR crc actual:%d expected:%d\n", crca, crc);
        return -2;
    }
    else
    {
        // CRC OK

        //DECODE RECORDS
        i = 8; // start at the 1st record
        //printf("openThings_decode(): message from %d, length:%d\n", *iDeviceId, length);

        while ((i < length) && (payload[i] != 0) && (record < OT_MAX_RECS))
        {
            // reset values
            memset(recs[record].retChar, '\0', 15);
            result = 0;

            // PARAM
            param = payload[i++];
            recs[record].wr = ((param & 0x80) == 0x80);
            recs[record].paramId = param & 0x7F;
            strcpy(recs[record].paramName, OTparams[openThings_getParamIndex(recs[record].paramId)].paramName);

            // no equivalent of 'in' for C, probably messy to code this here anyway as they are strings; do it in calling function instead (javascript in my case)
            /*
                if paramid in param_info:
                    paramname = (param_info[paramid])["n"] # name
                    paramunit = (param_info[paramid])["u"] # units
                else:
                    paramname = "UNKNOWN_" + hex(paramid)
                    paramunit = "UNKNOWN_UNIT"
                */

            // TYPE/LEN
            recs[record].typeId = payload[i] & 0xF0;
            rlen = payload[i++] & 0x0F;
            // printf("openThings_decode(): record[%d] paramid=0x%02x typeId=0x%02x values:[", record, recs[record].paramId, recs[record].typeId);

            // In C, it is not great at returning different types for a function; so we are just going to have to code it here rather than be a modular coder :(
            switch (recs[record].typeId)
            {
            case OT_CHAR:
                for (j = 0; j < rlen; j++)
                {
                    // printf("%d,", payload[i + j]);
                    recs[record].retChar[j] = payload[i + j];
                }
                recs[record].typeIndex = OTR_CHAR;
                break;
            case OT_UINT:
                for (j = 0; j < rlen; j++)
                {
                    // printf("%d,", payload[i + j]);
                    result <<= 8;
                    result += payload[i + j];
                }
                recs[record].typeIndex = OTR_INT;
                break;
            case OT_UINT4:
            case OT_UINT8:
            case OT_UINT12:
            case OT_UINT16:
            case OT_UINT20:
            case OT_UINT24:
                for (j = 0; j < rlen; j++)
                {
                    // printf("%d,", payload[i + j]);
                    result <<= 8;
                    result += payload[i + j];
                }
                // adjust BP
                recs[record].typeIndex = OTR_FLOAT;
                break;
            case OT_SINT:
            case OT_SINT8:
            case OT_SINT16:
            case OT_SINT24:
                for (j = 0; j < rlen; j++)
                {
                    // printf("%d,", payload[i + j]);
                    result <<= 8;
                    result += payload[i + j];
                } // turn to signed int based on high bit of MSB, 2's comp is 1's comp plus 1
                if ((payload[i] & 0x80) == 0x80)
                {
                    // negative
                    result = -(((!result) & ((2 ^ (length * 8)) - 1)) + 1);
                    //onescomp = (~result) & ((2**(length*8))-1)
                    //result = -(onescomp + 1)
                    // =result = -((~result) & ((2**(length*8))-1) + 1)
                }
                recs[record].typeIndex = OTR_INT;
                break;
            case OT_FLOAT:
                // TODO (@whaleygeek didnt do this either!)
                recs[record].typeIndex = -1;
                break;
            default:
                // TODO - are there other values?
                recs[record].typeIndex = -2;
            }

            // always store the integer result in the record
            recs[record].retInt = result;

            // Binary point adjustment (float)
            recs[record].retFloat = (float)result / pow(2, OTtypelen(recs[record].typeId));

            // printf("] typeIndex:%d Int:%d Float:%f Char:%s\n", recs[record].typeIndex, recs[record].retInt, recs[record].retFloat, recs[record].retChar);

            // move arrays on
            i += rlen;
            record++;
        }
    }

    // return the number of records
    return record;
}

/*
** openThings_switch()
** ===================
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
    unsigned char radio_msg[OTS_MSGLEN] = {OTS_MSGLEN - 1, ENERGENIE_MFRID, PRODUCTID_MIHO005, OT_DEFAULT_PIP, OT_DEFAULT_DEVICEID, OTC_SWITCH_OFF, 0x00, 0x00};

    printf("openThings_switch: productId=%d, deviceId=%d, state=%d\n", iProductId, iDeviceId, bSwitchState);

    /*
    ** Stage 1: Build the message to send
    */

    /* Stage 1a: OpenThings HEADER
    */
    // productId (usually 2 for MIHO005)
    radio_msg[OTH_INDEX_PRODUCTID] = iProductId;

    /*
    ** Stage 1b: OpenThings RECORDS (Commands)
    */
    // deviceId
    radio_msg[OTH_INDEX_DEVICEID] = (iDeviceId >> 16) & 0xFF;    //MSB
    radio_msg[OTH_INDEX_DEVICEID + 1] = (iDeviceId >> 8) & 0xFF; //MID
    radio_msg[OTH_INDEX_DEVICEID + 2] = iDeviceId & 0xFF;        //LSB

    if (bSwitchState)
    {
        // We already have the switch off command in the message, just override the switch value to on
        radio_msg[OT_INDEX_R1_VALUE] = 1;
    }

    /*
    ** Stage 1c: OpenThings FOOTER (CRC)
    */
    crc = calculateCRC(&radio_msg[5], (OTS_MSGLEN - 7));
    radio_msg[OTS_MSGLEN - 2] = ((crc >> 8) & 0xFF); // MSB
    radio_msg[OTS_MSGLEN - 1] = (crc & 0xFF);        // LSB

    // Stage 1d: encrypt body part of message
    cryptMsg(CRYPT_PID, CRYPT_PIP, &radio_msg[5], (OTS_MSGLEN - 5));

    /*
    ** Stage 2: Empty Rx buffer if required
    */
    ret = empty_radio_Rx_buffer();

    /*
    ** Stage 3: Transmit via radio adaptor, using mutex to block the radio
    */

    // mutex access radio adaptor
    if ((ret = lock_ener314rt()) != 0)
    {
        printf("openthings_switch(): error %d getting lock\n", ret);        
        return ret;

    } else {

        // Transmit encoded payload 26ms per payload * xmits
        radio_mod_transmit(RADIO_MODULATION_FSK, radio_msg, OTS_MSGLEN, xmits);

        // release mutex lock
        unlock_ener314rt();

    }

    return ret;
}
/*
** openThings_learn()
** =======
** Learn new FSK OpenThings based Energenie smart devices
** Currently this covers the 'MiHome Monitor', HiHome Adaptor Plus', 'MiHome Heating' TRV, ++++
**
** The OpenThings messages are comprised of 3 parts:
**  Header  - msgLength, manufacturerId, productId, encryptionPIP, and deviceId
**  Records - The body of the message, in this case a single command to switch the state
**  Footer  - CRC
**
** Functions performed include:
**    initialising the radio and setting the modulation
**    Setting radio to receive mode
**    receiving data via the ENER314-RT device
**    formatting and decoding the OpenThings FSK radio responses
**    returning array of learned devices as [productId:deviceId]
*/
unsigned char openThings_learn(unsigned char iTimeOut, char *devices)
{
    int ret = 0;
    uint8_t buf[MAX_FIFO_BUFFER];
    struct OTrecord OTrecs[OT_MAX_RECS];
    int OTpi;
    unsigned char mfrId, productId;
    unsigned int iDeviceId;
    int records;
    char deviceStr[100];

    printf("openthings_learn(): called\n");

    // reset devices response JSON
    strcpy(devices, "{\"devices\":[");

    // mutex access radio adaptor (for a while!)
    if ((ret = lock_ener314rt()) != 0)
    {
        printf("openthings_learn(): error %d getting lock\n", ret);
        return ret;
    }

    // Set FSK mode receive for OpenThings devices
    radio_setmode(RADIO_MODULATION_FSK, HRF_MODE_RECEIVER);

    // TODO - unlock

    // Loop until timeout
    do
    {
        sleep(1); // pause 1 second
        if (radio_is_receive_waiting())
        {
            //printf("openthings_learn(): radio_is_receive waiting\n");
            if (radio_get_payload_cbp(buf, MAX_FIFO_BUFFER) == RADIO_RESULT_OK)
            {
                // Received a valid payload, decode it
                records = openThings_decode(buf, &mfrId, &productId, &iDeviceId, OTrecs);
                if (records > 0)
                {
                    printf("Valid OpenThings Message. mfrId:%d productId:%d deviceId:%d records:%d\n", mfrId, productId, iDeviceId, records);

                    OTpi = openThings_getProductIndex(productId);

                    // Add to learned devices
                    sprintf(deviceStr, "{\"mfrId\":%d,\"productId\":%d,\"deviceId\":%d,\"control\":%d,\"product\":\"%s\"}", mfrId, productId, iDeviceId, OTproducts[OTpi].control, OTproducts[OTpi].product);
                    strcat(devices, deviceStr);

                    // add 1 to devices learned
                    ret++;
                }
                else
                {
                    printf("Invalid OT Payload, return=%d\n", records);
                }
            }
        }
        else
        {
            //printf("openthings_learn(%d): radio_is_receive waiting=FALSE\n",iTimeOut);
        }
    } while (iTimeOut-- > 0);

    //unlock mutex
    unlock_ener314rt();

    // Close JSON array
    sprintf(deviceStr, "],\"numDevices\":%d}", ret);
    strcat(devices, deviceStr);
    printf("Returning:\n%s\n", devices);

    //radio_standby();

    return ret;
}

/*
** openThings_receive()
** =======
** Receive a single FSK OpenThings message (if one waiting)
** Currently this covers the 'HiHome Adaptor Plus', 'MiHome Heating' TRV, ++++
**
** The OpenThings messages are comprised of 3 parts:
**  Header  - msgLength, manufacturerId, productId, encryptionPIP, and deviceId
**  Records - The body of the message, in this case a single command to switch the state
**  Footer  - CRC
**
** Functions performed include:
**    Setting radio to receive mode
**    receiving data via the ENER314-RT device
**    formatting and decoding the OpenThings FSK radio responses
**    returning array of returned parameters for a deviceId
*/
char openThings_receive(unsigned char iTimeOut, char *OTmsg)
{
    //int ret = 0;
    //uint8_t buf[MAX_FIFO_BUFFER];
    struct OTrecord OTrecs[OT_MAX_RECS];
    unsigned char mfrId, productId;
    unsigned int iDeviceId;
    int records, i;
    char OTrecord[100];

    //printf("openthings_receive(): called, msgPtr=%d\n", pRxMsgHead);

    // set default message for timeout value returned
    strcpy(OTmsg, "{\"deviceId\": 0}");

    // Clear data
    records = -1;
    iDeviceId = 0;

    /*
    ** Stage 1 - always clear the Rx buffer on the radio device (with locking)
    */
    i = empty_radio_Rx_buffer();
    printf("openThings_receive(): Rx buffer emptied of %d msg(s)", i);

    /*
    ** Stage 2 - decode and process next message in RxMsgs buffer
    */
    while (pRxMsgHead != pRxMsgTail)
    {
        {
            // message avaiable
            records = openThings_decode(RxMsgs[pRxMsgTail].msg, &mfrId, &productId, &iDeviceId, OTrecs);

            // move tail to next msg in buffer
            if (++pRxMsgTail == RX_MSGS)
                pRxMsgTail = 0;

            if (records > 0)
            {
                // Add to deviceList
                openThings_devicePut(iDeviceId, mfrId, productId);

                printf("Valid OpenThings Message. deviceId:%d mfrId:%d productId:%d recs:%d\n", iDeviceId, mfrId, productId, records);
                // build response JSON
                sprintf(OTmsg, "{\"deviceId\":%d,\"mfrId\":%d,\"productId\":%d,\"timestamp\":%d", iDeviceId, mfrId, productId, (int)RxMsgs[pRxMsgTail].t);

                // add records
                for (i = 0; i < records; i++)
                {
                    switch (OTrecs[i].typeIndex)
                    {
                    case OTR_CHAR: //CHAR
                        //sprintf(OTrecord, "{\"name\":\"%s\",\"id\":%d,\"value\":\"%s\"}",OTrecs[i].paramName, OTrecs[i].paramId, OTrecs[i].retChar);
                        sprintf(OTrecord, ",\"%s\":\"%s\"", OTrecs[i].paramName, OTrecs[i].retChar);
                        break;
                    case OTR_INT:
                        // sprintf(OTrecord, "{\"name\":\"%s\",\"id\":%d,\"value\":\"%d\"}",OTrecs[i].paramName, OTrecs[i].paramId, OTrecs[i].retInt);
                        sprintf(OTrecord, ",\"%s\":%d", OTrecs[i].paramName, OTrecs[i].retInt);
                        break;
                    case OTR_FLOAT:
                        // sprintf(OTrecord, "{\"name\":\"%s\",\"id\":%d,\"value\":\"%d\"}",OTrecs[i].paramName, OTrecs[i].paramId, OTrecs[i].retInt);
                        sprintf(OTrecord, ",\"%s\":%f", OTrecs[i].paramName, OTrecs[i].retFloat);
                    }

                    strcat(OTmsg, OTrecord);
                    // if ((i+1)<records){
                    //     // not last record add a ,
                    //     strcat(OTmsg,",");
                    // }
                }

                // close record array
                strcat(OTmsg, "}");
                // printf("openThings_receive: Returning\n%s",OTmsg);

                // valid message, break while loop
                break;
            }
        }

    } 

    //printf("openThings_receive: %d Returning:\n%s\n", iTimeOut, OTmsg);

    return records;
}

/*
** openThings_deviceList() - return list of known openThings devices
**
** deviceList is built up automatically by
**  - receive an OT payload
**  - learn a new device
**  - or by a manual poll if empty
**
*/
unsigned char openThings_deviceList(unsigned char iTimeOut, char *devices)
{
    int ret = 0;
    int i;
    char deviceStr[100];

    printf("openthings_deviceList(): called\n");

    if (NumDevices == 0)
    {
        // do learny

        // TODO
    }

    sprintf(devices, "{\"numDevices\":%d, \"devices\":[\n", NumDevices);

    for (i = 0; i < NumDevices; i++)
    {
        // add device to JSON
        sprintf(deviceStr, "{\"mfrId\":%d,\"productId\":%d,\"deviceId\":%d,\"control\":%d,\"product\":\"%s\"}",
                OTdevices[i].mfrId, OTdevices[i].productId, OTdevices[i].deviceId, OTdevices[i].control, OTdevices[i].product);
        strcat(devices, deviceStr);
        if (i + 1 < NumDevices)
        {
            // more records to come add a ',' to JSON array
            strcat(devices, ",\n");
        }
    }

    // close message
    strcat(devices, "]}");

    printf("Returning:\n%s\n", devices);

    return ret;
}

/*
** empty_radio_Rx_buffer() - empties the radio receive buffer of any messages into RxMsgs as quickly as possible
**
** Perform any locking required
** Always leave the radio in receive mode, as this could have been the first time we have been called
** unlock on completion
**
** returns the # of messages read, -1 if error getting lock
**
** TODO: Buffer is currently cyclic and destructive, we could loose messages, but I've made the assumption we always need
**       the latest messages
**
*/
int empty_radio_Rx_buffer()
{
    int i, recs = 0;

    // mutex access radio adaptor to set mode
    if ((i = lock_ener314rt()) != 0)
    {
        printf("empty_radio_Rx_buffer(): error %d getting lock\n", i);
        return -1;
    }
    else
    {
        // Set FSK mode receive for OpenThings devices (Energenie OOK devices dont generally transmit!)
        radio_setmode(RADIO_MODULATION_FSK, HRF_MODE_RECEIVER);

        i = 0;
        // do we have any messages waiting?
        while (radio_is_receive_waiting() && (i++ < RX_MSGS))
        {
            if (radio_get_payload_cbp(RxMsgs[pRxMsgHead].msg, MAX_FIFO_BUFFER) == RADIO_RESULT_OK)
            {
                recs++;
                // TODO: Only store valid OpenThings messages?

                // record message timestamp
                RxMsgs[pRxMsgHead].t = time(0);

                // wrap round buffer for next Rx
                if (++pRxMsgHead == RX_MSGS)
                    pRxMsgHead = 0;
            }
            else
            {
                printf("empty_radio_Rx_buffer(): error getting payload\n");
                break;
            }
        }
    }
    unlock_ener314rt();
    return recs;
};