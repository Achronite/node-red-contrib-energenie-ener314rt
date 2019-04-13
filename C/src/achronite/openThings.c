#include <stdio.h>
#include <stdbool.h>
#include <string.h>
//#include <unistd.h>
#include <math.h>
#include "openThings.h"
#include "lock_radio.h"
#include "../energenie/radio.h"
#include "../energenie/trace.h"
#include "../energenie/delay.h"

/*
** C module addition to energenie code to simplify the FSK OpenThings interaction with the Energenie ENER314-RT
** by minimising the number of calls required to interact with C radio device.
**
** Author: Phil Grainger - @Achronite, March 2019
*/

// OpenThings FSK paramters (known)  [{ParamName, paramId}]
// I've moved the likely ones to the top for speed, and included in .c file to prevent compiler warnings
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

// OpenThings FSK products (known)  [{mfrId, productId, control (boolean), product}]
static struct OT_PRODUCT OTproducts[NUM_OT_PRODUCTS] = {
    {4, 0x00, true,  "Unknown"       },
    {4, 0x01, false, "Monitor Plug"  },
    {4, 0x02, true,  "Adapter Plus"  },
    {4, 0x05, false, "House Monitor" },
    {4, 0x03, true,  "Radiator Valve"},
    {4, 0x0C, false, "Motion Sensor" },
    {4, 0x0D, false, "Open Sensor"   },
    {4, 0x0E, true,  "Thermostat"    }   // I dont know the productId of this yet, guessing at 0E
};

// Globals - yuck
unsigned short ran;
struct OT_DEVICE OTdevices[MAX_DEVICES]; // should maybe make this dynamic!
static int NumDevices = 0;               // number of auto-discovered OpenThings devices

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
void openThings_devicePut(unsigned int iDeviceId, unsigned char mfrId, unsigned char productId, bool joining)
{
    int OTpi;

    if (openThings_getDeviceIndex(iDeviceId) < 0)
    {
        // new device
        OTdevices[NumDevices].mfrId = mfrId;
        OTdevices[NumDevices].productId = productId;
        OTdevices[NumDevices].deviceId = iDeviceId;
        OTdevices[NumDevices].joined = !joining;

        // add product characteristics
        OTpi = openThings_getProductIndex(productId);
        OTdevices[NumDevices].control = OTproducts[OTpi].control;
        strcpy(OTdevices[NumDevices].product, OTproducts[OTpi].product);

#if defined(TRACE)
        TRACE_OUTS("openThings_devicePut() device added: ");
        TRACE_OUTN(NumDevices);
        TRACE_OUTC(':');
        TRACE_OUTN(iDeviceId);
        TRACE_NL();
#endif
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

    //printf("openThings_decode(): called\n");
    length = payload[0];

    // A good indication this is an OpenThings msg, is to check the length first, abort if too long or short
    if (length > MAX_FIFO_BUFFER || length < 10)
    {
        TRACE_OUTS("ERROR openThings_decode(): Not OT Message, invalid length=");
        TRACE_OUTN(length);
        TRACE_NL();
        return -1;
    }

    // DECODE HEADER
    *mfrId = payload[1];
    *productId = payload[2];
    pip = (payload[3] << 8) + payload[4];

    //struct sOTHeader = { length, mfrId, productId, pip };
#if defined(TRACE)
    printf("openThings_decode(): length=%d, mfrId=%d, productId=%d, pip=%d", length, *mfrId, *productId, pip);
#endif

    // decode encrypted body (destructive - watch for length errors!)
    cryptMsg(CRYPT_PID, pip, &payload[5], (length - 4));
    *iDeviceId = (payload[5] << 16) + (payload[6] << 8) + payload[7];

#if defined(TRACE)
    printf(", deviceId=%d\n", *iDeviceId);
#endif

    // CHECK CRC from last 2 bytes of message
    crca = (payload[length - 1] << 8) + payload[length];
    crc = calculateCRC(&payload[5], (length - 6));

    if (crc != crca)
    {
        // CRC does not match
        TRACE_OUTS("openThings_decode(%d): Not OT Message, CRC error\n");
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
            //printf("openThings_decode(): record[%d] paramid=0x%02x typeId=0x%02x values:[", record, recs[record].paramId, recs[record].typeId);

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

            //printf("] typeIndex:%d Int:%d Float:%f Char:%s\n", recs[record].typeIndex, recs[record].retInt, recs[record].retFloat, recs[record].retChar);

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

#if defined(TRACE)
    printf("openThings_switch: productId=%d, deviceId=%d, state=%d\n", iProductId, iDeviceId, bSwitchState);
#endif
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

    // mutex access radio adaptor to set mode
    if ((ret = lock_ener314rt()) != 0)
    {
        TRACE_FAIL("openthings_switch(): error getting lock\n");
        return -1;
    }
    else
    {
        ret = empty_radio_Rx_buffer(DT_CONTROL);
        //printf("openThings_switch(%d): Rx_Buffer ", ret);

        /*
        ** Stage 3: Transmit via radio adaptor, using mutex to block the radio
        */
        // Transmit encoded payload 26ms per payload * xmits
        radio_mod_transmit(RADIO_MODULATION_FSK, radio_msg, OTS_MSGLEN, xmits);

        // release mutex lock
        unlock_ener314rt();
    }

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
char openThings_receive(char *OTmsg)
{
    //int ret = 0;
    //uint8_t buf[MAX_FIFO_BUFFER];
    struct OTrecord OTrecs[OT_MAX_RECS];
    unsigned char mfrId, productId;
    unsigned int iDeviceId;
    int records, i;
    char OTrecord[100];
    struct RADIO_MSG rxMsg;
    bool joining = false;

    //printf("openthings_receive(): called, msgPtr=%d\n", pRxMsgHead);

    // set default message if no message available
    strcpy(OTmsg, "{\"deviceId\": 0}");

    // Clear data
    records = -1;
    iDeviceId = 0;

    /*
    ** Stage 1 - always clear the Rx buffer on the radio device (with locking)
    */
    if ((i = lock_ener314rt()) != 0)
    {
        TRACE_FAIL("openthings_switch(): error getting lock\n");
        return -1;
    }
    else
    {
        i = empty_radio_Rx_buffer(DT_MONITOR);
        unlock_ener314rt();
    }

    /*
    ** Stage 2 - decode and process next message in RxMsgs buffer
    */
    //printf("<%d-%d>",pRxMsgHead, pRxMsgTail);
    while (pop_RxMsg(&rxMsg) >= 0)
    {
        // message avaiable
        //printf("openThings_receive(): msg popped, ts=%d\n", (int)rxMsg.t);
        records = openThings_decode(rxMsg.msg, &mfrId, &productId, &iDeviceId, OTrecs);

        if (records > 0)
        {
            //printf("openThings_receive(): Valid OT: \n");
            // build response JSON
            sprintf(OTmsg, "{\"deviceId\":%d,\"mfrId\":%d,\"productId\":%d,\"timestamp\":%d", iDeviceId, mfrId, productId, (int)rxMsg.t);

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
                    if (OTrecs[i].paramId == 0x6A)
                        joining = true;
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

            // Add to deviceList
            openThings_devicePut(iDeviceId, mfrId, productId, joining);

            // valid message, break while loop
            break;
        }
    }

    TRACE_OUTS("openThings_receive: Returning:\n");
    TRACE_OUTS(OTmsg);

    return records;
}

/*
** openThings_deviceList() - return list of known openThings devices
**
** deviceList is built up automatically by
**  - receive an OT payload
**  - learn a new device
**  - or by a manual poll if empty**
*/
unsigned char openThings_deviceList(char *devices, bool scan)
{
    int ret = 0;
    int i;
    char deviceStr[100];

    TRACE_OUTS("openthings_deviceList(): called\n");

    if (NumDevices == 0 || scan)
    {
        // We dont have any learnt devices :( Need to run a scan for a bit
        openthings_scan(10);
    }

    sprintf(devices, "{\"numDevices\":%d, \"devices\":[\n", NumDevices);

    for (i = 0; i < NumDevices; i++)
    {
        // add device to JSON
        sprintf(deviceStr, "{\"mfrId\":%d,\"productId\":%d,\"deviceId\":%d,\"control\":%d,\"product\":\"%s\",\"joined\":%d}",
                OTdevices[i].mfrId, OTdevices[i].productId, OTdevices[i].deviceId, OTdevices[i].control, OTdevices[i].product, OTdevices[i].joined);
        strcat(devices, deviceStr);
        if (i + 1 < NumDevices)
        {
            // more records to come add a ',' to JSON array
            strcat(devices, ",\n");
        }
    }

    // close message
    strcat(devices, "]}");

    TRACE_OUTS("openthings_deviceList(): Returning: ");
    TRACE_OUTS(devices);
    TRACE_NL();

    return ret;
}

/*
** openthings_scan() - listen for valid openThings messages until iTimeOut passed
**                     used to discover devices when we have not autodiscovered any or a search is forced in GUI
**
** This is blocking on the UI and the radio, so should only be performed when necessary
** Also adds FSK devices that are in learning mode (5 second button press to initiate)
**
*/
void openthings_scan(int iTimeOut)
{
    struct OTrecord OTrecs[OT_MAX_RECS];
    unsigned char mfrId, productId;
    unsigned int iDeviceId;
    int records, i;
    //char OTrecord[100];
    struct RADIO_MSG rxMsg;
    bool joining = false;

    // Clear data
    records = 0;
    iDeviceId = 0;

    /*
    ** Stage 1 - fill the Rx Buffer (with locking between calls)
    */

    // do a few calls to switch to initiate monitor mode and populate the RxBuffer
    for (i = 0; i < iTimeOut; i++)
    {
        if ((lock_ener314rt()) == 0)
        {
            records += empty_radio_Rx_buffer(DT_LEARN);
            unlock_ener314rt();
            if (records >= RX_MSGS)
                break;
        }
        // wait for more messages
        if (i + 1 < iTimeOut)
            delaysec(1);
    }

    /*
    ** Stage 2 - peek ALL the messages in RxMsgs buffer; this is non-destructive
    */
    for (i = 0; i < RX_MSGS; i++)
    {
        if (get_RxMsg(i, &rxMsg) > 0)
        {
            // message avaiable
            //printf("openThings_scan(): msg got, ts=%d\n", (int)rxMsg.t);
            records = openThings_decode(rxMsg.msg, &mfrId, &productId, &iDeviceId, OTrecs);

            if (records > 0)
            {
                joining = false;
                //printf("openThings_scan(): Valid OT: deviceId:%d mfrId:%d productId:%d recs:%d\n", iDeviceId, mfrId, productId, records);

                // scan records for JOIN requests, and reply to add
                for (i = 0; i < records; i++)
                {
                    if (OTrecs[i].paramId == 0x6A)
                    {
                        TRACE_OUTS("openThings_scan(): New device found, sending ACK: deviceId:");
                        TRACE_OUTN(iDeviceId);
                        TRACE_NL();
                        joining = true;
                        i = openThings_joinACK(productId, iDeviceId, 20);
                    }
                }
                // Add devices to standard deviceList
                openThings_devicePut(iDeviceId, mfrId, productId, joining);
            }
        }
    }
}

/*
** openThings_joinACK()
** ===================
** Send a JOIN ACK message to a FSK OpenThings based Energenie smart device
**
** THIS FUNCTION DOES NOT WORK!
**
** The OpenThings messages are comprised of 3 parts:
**  Header  - msgLength, manufacturerId, productId, encryptionPIP, and deviceId
**  Records - The body of the message, in this case a single command to switch the state
**  Footer  - CRC
**
** Functions performed include:
**    encoding of the device and join request
**    formatting and encoding the OpenThings FSK radio request
**    sending the radio request via the ENER314-RT RaspberryPi adaptor
*/
unsigned char openThings_joinACK(unsigned char iProductId, unsigned int iDeviceId, unsigned char xmits)
{
    int ret = 0;
    unsigned short crc;
    unsigned char radio_msg[OTA_MSGLEN] = {OTA_MSGLEN - 1, ENERGENIE_MFRID, PRODUCTID_MIHO005, OT_DEFAULT_PIP, OT_DEFAULT_DEVICEID, OTC_JOIN_ACK, 0x00, 0x00};

    // "recs": [
    //     {
    //         "wr":      False,
    //         "paramid": OpenThings.PARAM_JOIN,  0x6A
    //         "typeid":  OpenThings.Value.UINT,  0x00
    //         "length":  0
    //     }
    // ]

    printf("openThings_joinACK(): productId=%d, deviceId=%d\n", iProductId, iDeviceId);

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

    /*
    ** Stage 1c: OpenThings FOOTER (CRC)
    */
    crc = calculateCRC(&radio_msg[5], (OTA_MSGLEN - 7));
    radio_msg[OTA_MSGLEN - 2] = ((crc >> 8) & 0xFF); // MSB
    radio_msg[OTA_MSGLEN - 1] = (crc & 0xFF);        // LSB

    // Stage 1d: encrypt body part of message
    cryptMsg(CRYPT_PID, CRYPT_PIP, &radio_msg[5], (OTA_MSGLEN - 5));

    /*
    ** Stage 2: Empty Rx buffer if required
    */

    // mutex access radio adaptor to set mode
    if ((ret = lock_ener314rt()) != 0)
    {
        return -1;
    }
    else
    {
        /*
        ** Stage 3: Transmit via radio adaptor, using mutex to block the radio
        */
        // Transmit encoded payload 26ms per payload * xmits
        radio_mod_transmit(RADIO_MODULATION_FSK, radio_msg, OTA_MSGLEN, xmits);

        // release mutex lock
        unlock_ener314rt();
    }

    return ret;
}