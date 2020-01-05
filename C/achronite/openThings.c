#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <sys/time.h>
#include "openThings.h"
#include "lock_radio.h"
#include "../energenie/radio.h"
#include "../energenie/hrfm69.h"
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
    {"UNKNOWN", 0x00},
    {"FREQUENCY", 0x66},
    {"REAL_POWER", 0x70},
    {"REACTIVE_POWER", 0x71},
    {"VOLTAGE", 0x76},
    {"TEMPERATURE", OTP_TEMPERATURE},
    {"DIAGNOSTICS", 0x26},
    {"ALARM", 0x21},
    {"DEBUG_OUTPUT", 0x2D},
    {"IDENTIFY", 0x3F},
    {"SOURCE_SELECTOR", 0x40}, // write only
    {"WATER_DETECTOR", 0x41},
    {"GLASS_BREAKAGE", 0x42},
    {"CLOSURES", 0x43},
    {"DOOR_BELL", 0x44},
    {"ENERGY", 0x45},
    {"FALL_SENSOR", 0x46},
    {"GAS_VOLUME", 0x47},
    {"AIR_PRESSURE", 0x48},
    {"ILLUMINANCE", 0x49},
    {"LEVEL", 0x4C},
    {"RAINFALL", 0x4D},
    {"APPARENT_POWER", 0x50},
    {"POWER_FACTOR", 0x51},
    {"REPORT_PERIOD", 0x52},
    {"SMOKE_DETECTOR", 0x53},
    {"TIME_AND_DATE", 0x54},
    {"VIBRATION", 0x56},
    {"WATER_VOLUME", 0x57},
    {"WIND_SPEED", 0x58},
    {"GAS_PRESSURE", 0x61},
    {"BATTERY_LEVEL", 0x62},
    {"CO_DETECTOR", 0x63},
    {"DOOR_SENSOR", 0x64},
    {"EMERGENCY", 0x65},
    {"GAS_FLOW_RATE", 0x67},
    {"REL_HUMIDITY", 0x68},
    {"CURRENT", 0x69},
    {"JOIN", 0x6A},
    {"LIGHT_LEVEL", 0x6C},
    {"MOTION_DETECTOR", 0x6D},
    {"OCCUPANCY", 0x6F},
    {"ROTATION_SPEED", 0x72},
    {"SWITCH_STATE", 0x73},
    {"WATER_FLOW_RATE", 0x77},
    {"WATER_PRESSURE", 0x78},
    {"TEST", 0xAA}};

// OpenThings FSK products (known)  [{mfrId, productId, control (0=no, 1=yes, 2=cached), product}]
static struct OT_PRODUCT OTproducts[NUM_OT_PRODUCTS] = {
    {4, 0x00, 1, "Unknown"},
    {4, 0x01, 0, "Monitor Plug"},
    {4, 0x02, 1, "Adapter Plus"},
    {4, 0x03, 2, "Radiator Valve"},
    {4, 0x05, 0, "House Monitor"},
    {4, 0x0C, 0, "Motion Sensor"},
    {4, 0x0D, 0, "Open Sensor"},
    {4, 0x0E, 1, "Thermostat"} // I dont know the productId of this yet, guessing at 0E
};

// Globals - yuck
unsigned short g_ran;
struct OT_DEVICE g_OTdevices[MAX_DEVICES]; // should maybe make this dynamic!
static int g_NumDevices = 0;               // number of auto-discovered OpenThings devices
static int g_CachedCmds = 0;               // number of eTRV devices with commands waiting to be sent to them (controls Rx loop behaviour)

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
        if ((g_ran & 0x01) != 0)
        {
            // bit0 set
            g_ran = ((g_ran >> 1) ^ 62965);
        }
        else
        {
            // bit0 clear
            g_ran = g_ran >> 1;
        }
    }
    return (g_ran ^ data ^ 90);
}

/*
** cryptMsg() - en/decrypt an OpenThings message (destructive)
**
** Code converted from python module by @whaleygeek
*/
void cryptMsg(unsigned char pid, unsigned short pip, unsigned char *msg, unsigned int length)
{
    unsigned char i;

    // old whaleygeek
    //g_ran = (((pid & 0xFF) << 8) ^ pip);

    // new from gpbenton
    g_ran = ((((unsigned short)pid) << 8) ^ pip);

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

/* openThings_getDeviceIndex() - finds the id in the g_OTdevices array and returns index if it exists, otherwise return -1
*/
int openThings_getDeviceIndex(unsigned int id)
{
    for (int i = 0; i < g_NumDevices; i++)
    {
        if (g_OTdevices[i].deviceId == id)
            return i;
    }
    return -1;
}

/*
** openThings_devicePut() - add device to deviceList if it is not already there, return the index
*/
int openThings_devicePut(unsigned int iDeviceId, unsigned char mfrId, unsigned char productId, bool joining)
{
    int OTpi, OTdi;

    OTdi = openThings_getDeviceIndex(iDeviceId);
    if (OTdi < 0)
    {
        // new device
        OTdi = g_NumDevices;
        g_OTdevices[OTdi].mfrId = mfrId;
        g_OTdevices[OTdi].productId = productId;
        g_OTdevices[OTdi].deviceId = iDeviceId;
        g_OTdevices[OTdi].joined = !joining;

        // add product characteristics
        OTpi = openThings_getProductIndex(productId);
        g_OTdevices[OTdi].control = OTproducts[OTpi].control;
        strcpy(g_OTdevices[OTdi].product, OTproducts[OTpi].product);

        // add extra structure if it is an eTRV
        if (productId == PRODUCTID_MIHO013)
        {
            g_OTdevices[OTdi].trv = malloc(sizeof(struct TRV_DEVICE));
            if (g_OTdevices[OTdi].trv != NULL)
            {
                // malloc OK, set defaults for trv
                g_OTdevices[OTdi].trv->valve = UNKNOWN;
                g_OTdevices[OTdi].trv->cachedCmd[0] = '\0';
                g_OTdevices[OTdi].trv->voltageDate = 0;
                g_OTdevices[OTdi].trv->diagnosticDate = 0;
                g_OTdevices[OTdi].trv->valveDate = 0;
                g_OTdevices[OTdi].trv->retries = 0;
                g_OTdevices[OTdi].trv->diagnostics = 0;
                g_OTdevices[OTdi].trv->voltage = 0;
                g_OTdevices[OTdi].trv->targetC = 0;
                g_OTdevices[OTdi].trv->errors = false;
                g_OTdevices[OTdi].trv->lowPowerMode = false;
                /*     
    float         currentC;
    bool          lowPowerMode;
    unsigned char command;
*/
            }
        }
        else
        {
            // this isnt a trv, so we dont need an extra struct
            g_OTdevices[OTdi].trv = NULL;
        }

#if defined(TRACE)
        TRACE_OUTS("openThings_devicePut() device added: ");
        TRACE_OUTN(OTdi);
        TRACE_OUTC(':');
        TRACE_OUTN(iDeviceId);
        TRACE_NL();
#endif
        g_NumDevices++;
    }
    // else
    // {
    //     printf("openThings_devicePut() device %d already exist\n", iDeviceId);
    // }
    return OTdi;
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
**   Decoding the received OpenThings message
**   Sending any outstanding commands to an eTRV ASAP
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

    // A good indication this is an OpenThings msg, is to check the length first, abort if too long or short
    if (length > MAX_FIFO_BUFFER || length < 10)
    {
        /*
        TRACE_OUTS("ERROR openThings_decode(): Not OT Message, invalid length=");
        TRACE_OUTN(length);
        TRACE_NL();
        */
        return -1;
    }

    // DECODE HEADER
    *mfrId = payload[1];
    *productId = payload[2];
    pip = (unsigned short)((payload[OTH_INDEX_PIP] << 8) | payload[OTH_INDEX_PIP + 1]);

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
        //TRACE_OUTS("openThings_decode(%d): Not OT Message, CRC error\n");
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
            //memset(recs[record].retChar, '\0', 15);
            result = 0;

            // PARAM
            param = payload[i++];
            recs[record].wr = ((param & 0x80) == 0x80);
            recs[record].paramId = param & 0x7F;

            //
            // If we have received a TEMPERATURE (OTP_TEMPERATURE) parameter for an eTRV (3), we need to send any outstanding messages to it ASAP as it only has a small Rx window
            //
            if ((recs[record].paramId == OTP_TEMPERATURE) && (*productId == 3))
            {
                openThings_cache_send(*iDeviceId);
            }

            int paramIndex = openThings_getParamIndex(recs[record].paramId);
            if (paramIndex != 0)
            {
                strcpy(recs[record].paramName, OTparams[paramIndex].paramName);
            }
            else
            {
                sprintf(recs[record].paramName, "UNKNOWN_0x%2x", recs[record].paramId);
            }
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

            if (rlen > 0)
            {
                // str can cause probs clear it
                memset(recs[record].retChar, 0, 15);

                // set MSB always to reduce loops below
                result = payload[i];

                // In C, it is not great at returning different types for a function; so we are just going to have to code it here rather than be a modular coder :(
                switch (recs[record].typeId)
                {
                case OT_CHAR:
                    // memcpy is faster
                    memcpy(recs[record].retChar, &payload[i], rlen);
                    /*
                for (j = 0; j < rlen; j++)
                {
                    // printf("%d,", payload[i + j]);
                    recs[record].retChar[j] = payload[i + j];
                }
                */
                    recs[record].typeIndex = OTR_CHAR;
                    break;
                case OT_UINT:
                    for (j = 1; j < rlen; j++)
                    {
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
                    for (j = 1; j < rlen; j++)
                    {
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
                    for (j = 1; j < rlen; j++)
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
            }
            else
            {
                TRACE_OUTS("rlen=0\n");
                recs[record].retInt = 0;
            }

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
    unsigned short crc, pip;
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

    // pip (random)
    radio_msg[OTH_INDEX_PIP] = rand();
    radio_msg[OTH_INDEX_PIP + 1] = rand();
    pip = (unsigned short)((radio_msg[OTH_INDEX_PIP] << 8) | radio_msg[OTH_INDEX_PIP + 1]);

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

#if defined(TRACE)
    TRACE_OUTS("switch tx payload (unencrypted):\n");
    for (int i = 0; i < OTS_MSGLEN; i++)
    {
        TRACE_OUTN(radio_msg[i]);
        TRACE_OUTC(',');
    }
    TRACE_NL();
#endif

    // Stage 1d: encrypt body part of message, using the stored pip
    cryptMsg(CRYPT_PID, pip, &radio_msg[5], (OTS_MSGLEN - 5));

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
** openThings_build_msg()
** ===================
** Creates a fully-formed radio message to be sent to an FSK OpenThings based device
** Message is not sent here
**
** TODO: Make this the only msg builder and sender, caching if device requires it
**
** Currently this has been tested with 'HiHome Adaptor Plus' and 'MiHome Heating' TRV
** Other OpenThings devices should work
**
** SUPPORTED Commands (x)
** ----------------------
** x OTCP_SWITCH_STATE              0xF3  Set status of switched device
** - OTCP_JOIN                      0xEA  Acknowledge a JOIN request
** x OTCP_TEMP_SET                  0xF4  Send new target temperature
** x OTCP_EXERCISE_VALVE            0xA3  Send exercise valve command to TRV 
** x OTCP_REQUEST_VOLTAGE           0xE2  Request battery voltage 
** x OTCP_REQUEST_DIAGNOTICS        0xA6  Request diagnostic flags
** x OTCP_SET_VALVE_STATE           0xA5  Set TRV valve state
** - OTCP_SET_LOW_POWER_MODE        0xA4  Set TRV 0=Low power mode off, 1=Low power mode on
** - OTCP_SET_REPORTING_INTERVAL    0xD2  Update reporting interval to requested value
**
** The OpenThings messages are comprised of 3 parts:
**  Header  - msgLength, manufacturerId, productId, encryptionPIP, and deviceId
**  Records - The body of the message, in this case a single command to switch the state
**  Footer  - CRC
**
** Functions performed include:
**  NO initialising the radio and setting the modulation
**     encoding of the device and command
**     formatting and encoding the OpenThings FSK radio request
**  NO sending the radio request via the ENER314-RT RaspberryPi adaptor
**     returning built message
*/
int openThings_build_msg(unsigned char iProductId, unsigned int iDeviceId, unsigned char iCommand, unsigned int iData, unsigned char *radio_msg)
{
    int ret = 0;
    unsigned short crc, pip;
    //unsigned char radio_msg[MAX_R1_MSGLEN] = {0x00, ENERGENIE_MFRID, PRODUCTID_MIHO005, OT_DEFAULT_PIP, OT_DEFAULT_DEVICEID, 0x00, 0x00, 0x00, 0x00};
    unsigned char msglen = 0;
    unsigned char iType = 0x00;

#if defined(TRACE)
    printf("openThings_build_msg: productId=%d, deviceId=%d, cmd=%d, data=%d cmd=", iProductId, iDeviceId, iCommand, iData);
#endif

    switch (iCommand)
    {
    case OTCP_SET_VALVE_STATE:
        msglen = MIN_R1_MSGLEN + 1;
        iType = 0x01;
#if defined(TRACE)
        printf("SET_VALVE_STATE msglen=%d\n", msglen);
#endif
        break;

    case OTCP_TEMP_SET:
        msglen = MIN_R1_MSGLEN + 2;
        iType = 0x92; // bit weird, but it works
#if defined(TRACE)
        printf("OTCP_TEMP_SET msglen=%d\n", msglen);
#endif
        break;

    case OTCP_REQUEST_DIAGNOTICS:
        msglen = MIN_R1_MSGLEN;
#if defined(TRACE)
        printf("OTCP_REQUEST_DIAGNOTICS msglen=%d\n", msglen);
#endif
        break;

    case OTCP_EXERCISE_VALVE:
        msglen = MIN_R1_MSGLEN;
#if defined(TRACE)
        printf("OTCP_EXERCISE_VALVE msglen=%d\n", msglen);
#endif
        break;

    case OTCP_REQUEST_VOLTAGE:
        msglen = MIN_R1_MSGLEN;
#if defined(TRACE)
        printf("OTCP_REQUEST_VOLTAGE msglen=%d\n", msglen);
#endif
        break;

    case OTCP_SWITCH_STATE:
        msglen = MIN_R1_MSGLEN + 1;
        iType = 0x01;
#if defined(TRACE)
        printf("OTCP_SWITCH_STATE msglen=%d\n", msglen);
#endif
        break;

    case OTCP_IDENTIFY:
        msglen = MIN_R1_MSGLEN;
#if defined(TRACE)
        printf("OTCP_IDENTIFY msglen=%d\n", msglen);
#endif
        break;

    case OTCP_SET_LOW_POWER_MODE:
        msglen = MIN_R1_MSGLEN + 1;
        iType = 0x01;
#if defined(TRACE)
        printf("OTCP_SET_LOW_POWER_MODE msglen=%d\n", msglen);
#endif
        break;

    default:
        // unknown command, abort
        return -1;
    }

    /*
    ** Stage 1: Build the message to send
    */

    /* Stage 1a: OpenThings HEADER
    */
    // message length
    radio_msg[0] = msglen - 1;

    // product
    radio_msg[OTH_INDEX_MFRID] = ENERGENIE_MFRID;
    radio_msg[OTH_INDEX_PRODUCTID] = iProductId;

    // pip random
    radio_msg[OTH_INDEX_PIP] = rand();
    radio_msg[OTH_INDEX_PIP + 1] = rand();
    pip = (unsigned short)((radio_msg[OTH_INDEX_PIP] << 8) | radio_msg[OTH_INDEX_PIP + 1]);

    /*
    ** Stage 1b: Build OpenThings RECORDS (Commands)
    */
    // deviceId
    radio_msg[OTH_INDEX_DEVICEID] = (iDeviceId >> 16) & 0xFF;    //MSB
    radio_msg[OTH_INDEX_DEVICEID + 1] = (iDeviceId >> 8) & 0xFF; //MID
    radio_msg[OTH_INDEX_DEVICEID + 2] = iDeviceId & 0xFF;        //LSB

    // command
    radio_msg[OT_INDEX_R1_CMD] = iCommand;

    // command data type
    radio_msg[OT_INDEX_R1_TYPE] = iType;

    // data value, base encoding off the msglen
    switch (msglen - MIN_R1_MSGLEN)
    {
    case 0:
        break;
    case 1:
        radio_msg[OT_INDEX_R1_VALUE] = iData & 0xFF;
        break;
    case 2:
        radio_msg[OT_INDEX_R1_VALUE] = iData & 0xFF;
        radio_msg[OT_INDEX_R1_VALUE + 1] = (iData >> 8) & 0xFF;
        break;
    }

    /*
    ** Stage 1c: OpenThings FOOTER (CRC)
    */
    crc = calculateCRC(&radio_msg[5], (msglen - 7));
    radio_msg[msglen - 2] = ((crc >> 8) & 0xFF); // MSB
    radio_msg[msglen - 1] = (crc & 0xFF);        // LSB

#if defined(TRACE)
    TRACE_OUTS("Built payload (unencrypted): ");
    for (int i = 0; i < msglen; i++)
    {
        TRACE_OUTN(radio_msg[i]);
        TRACE_OUTC(',');
    }
    TRACE_NL();
#endif

    // Stage 1d: encrypt body part of message
    cryptMsg(CRYPT_PID, pip, &radio_msg[5], (msglen - 5));

#if defined(TRACE)
    TRACE_OUTS("Built payload (encrypted): ");
    for (int i = 0; i < msglen; i++)
    {
        TRACE_OUTN(radio_msg[i]);
        TRACE_OUTC(',');
    }
    TRACE_NL();
#endif

    return ret;
}

/*
** openThings_cache_cmd()
** ===================
** Cache a command to be sent to a 'Control and Monitor' RF FSK OpenThings based Energenie smart device
** This is designed for devices that have a small receive window such as the 'MiHome Heating' TRV
**
** Build the full message here, as Rx window is quite small for eTRV
**
*/
char openThings_cache_cmd(unsigned int iDeviceId, unsigned char command, unsigned int data)
{
    int ret = 0, index;
    unsigned char radio_msg[MAX_R1_MSGLEN] = {0};

#if defined(TRACE)
    printf("openThings_cache_cmd(): deviceId=%d, cmd=%d, value=%d\n", iDeviceId, command, data);
#endif

    /*
    ** Only eTRVs need cached commands today, so store the command in the trv specific struct for the OTdevice array if we have record of it
    ** (i.e. we have had an Rx a msg from it)
    */
    index = openThings_getDeviceIndex(iDeviceId);
    if (index >= 0)
    {
        // Device known, build full radio message
        ret = openThings_build_msg(g_OTdevices[index].productId, iDeviceId, command, data, radio_msg);

        if (ret == 0)
        {
            // store message against the Device array, only 1 cached command is supported at any one time
            if (g_OTdevices[index].trv->retries <= 0)
            {
                g_CachedCmds++; // record that we have g_CachedCmds for this device

                // belt and braces
                if (g_CachedCmds < 1)
                    g_CachedCmds = 1;
            }

            memcpy(g_OTdevices[index].trv->cachedCmd, radio_msg, MAX_R1_MSGLEN);
            g_OTdevices[index].trv->command = command;
            g_OTdevices[index].trv->retries = TRV_TX_RETRIES; // Rx window is really small, so retry the Tx this number of times

            // Store any output only variables in eTRV state
            switch (command)
            {
            case OTCP_TEMP_SET:
                g_OTdevices[index].trv->targetC = data;
                break;
            case OTCP_SWITCH_STATE:
                g_OTdevices[index].trv->valve = data;
            }
            TRACE_OUTN(g_CachedCmds);
            TRACE_OUTS(" payload(s) cached\n");
        }
    }
    else
    {
        // TODO: Support caching before device known
        TRACE_OUTS("openThings_cache_cmd() ERROR: unable to cache command for unknown device.\n");
        ret = -2;
    }

    return ret;
}

/*
** openThings_receive()
** =======
** Receive a single FSK OpenThings message.  This function has 2 modes:
**   - if timeout > 0 then the function will wait 'timeout' ms or until a valid message is received
**   - if timeout = 0 then the function will return immediately, even if there is no valid message received
**
** This node is designed for all 'monitor' & 'control & monitor' nodes, including the 'HiHome Adaptor Plus' and MiHome Heating'
**
** An OpenThings message is comprised of 3 parts:
**  Header  - msgLength, manufacturerId, productId, encryptionPIP, and deviceId
**  Records - The body of the message, which can contain multiple parameters (records) returned
**  Footer  - CRC
**
** Functions performed include:
**   - mutex locking radio adaptor during radio operations
**   - Setting radio to receive mode
**   - receiving data via the ENER314-RT device
**   - formatting and decoding the OpenThings FSK radio responses
**   - auto add any devices to device list, responding to join requests if applicable
**   - If a cached command is outstanding for a device that only has a small receive window (e.g. eTRV), send the command
**   - returning JSON for the received msg OR returning '{"deviceId": 0}' if no msg available
**
** TODO (optimisations):
*/
int openThings_receive(char *OTmsg, unsigned int buflen, unsigned int timeout)
{
    //int ret = 0;
    //uint8_t buf[MAX_FIFO_BUFFER];
    struct OTrecord OTrecs[OT_MAX_RECS];
    unsigned char mfrId, productId;
    unsigned int iDeviceId;
    int records, i, msgsInRxBuf;
    char OTrecord[200];
    struct RADIO_MSG rxMsg;
    bool joining = false;
    int OTdi;
    struct timeval startTime, currentTime, diffTime;
    unsigned int diff = 0;

    //printf("openthings_receive(): called, buflen=%d\n", buflen);

    //record startTime for timeout
    if (timeout > 0)
    {
        gettimeofday(&startTime, NULL);
    }
    else
    {
        diff = 0;
    }

    // set default message if no message available
    strcpy(OTmsg, "{\"deviceId\": 0}");

    // 2 nested loops here, the plan is to wait until we have a valid message or the 'timeout' is reached:
    //  - the 1st loop empties the radio buffer
    //  - the end loops until buffer empty or we have a valid OT msg
    do
    {
        // Clear data
        records = -1;
        iDeviceId = 0;

        /*
        ** Stage 1 - empty the Rx buffer on the radio device (with locking)
        */
        if ((i = lock_ener314rt()) == 0)
        {
            i = empty_radio_Rx_buffer(DT_MONITOR);
            unlock_ener314rt();
        } else {
            // probably been asked to close quit loop
            return -3;
        }

        /*
        ** Stage 2 - decode and process next message in RxMsgs buffer
        */
        //printf("<%d-%d>",pRxMsgHead, pRxMsgTail);

        // loop2 - until we have read a valid OTmsg OR the RxMsg buffer is empty
        do
        {
            if ((msgsInRxBuf = pop_RxMsg(&rxMsg)) >= 0)
            {
                // Rx message avaiable in buffer
                //printf("openThings_receive(): msg popped, ts=%d\n", (int)rxMsg.t);
                records = openThings_decode(rxMsg.msg, &mfrId, &productId, &iDeviceId, OTrecs);

                if (records > 0)
                {
                    // build response JSON
                    sprintf(OTmsg, "{\"deviceId\":%d,\"mfrId\":%d,\"productId\":%d,\"timestamp\":%d", iDeviceId, mfrId, productId, (int)rxMsg.t);

                    // add records
                    for (i = 0; i < records; i++)
                    {
#if defined(FULLTRACE)
                        TRACE_OUTS("openThings_receive(): rec:");
                        TRACE_OUTN(i);
                        sprintf(OTrecord, " {\"name\":\"%s\",\"id\":%d,\"type\":%d,\"str\":\"%s\",\"int\":%d,\"float\":%f}\n", OTrecs[i].paramName, OTrecs[i].paramId, OTrecs[i].typeIndex, OTrecs[i].retChar, OTrecs[i].retInt, OTrecs[i].retFloat);
                        TRACE_OUTS(OTrecord);
#endif
                        switch (OTrecs[i].typeIndex)
                        {
                        case OTR_CHAR: //CHAR
                            sprintf(OTrecord, ",\"%s\":\"%s\"", OTrecs[i].paramName, OTrecs[i].retChar);
                            break;
                        case OTR_INT:
                            sprintf(OTrecord, ",\"%s\":%d", OTrecs[i].paramName, OTrecs[i].retInt);

                            // Special record processing
                            switch (OTrecs[i].paramId)
                            {
                            case OTP_JOIN: // JOIN_ACK
                                // We seem to have stumbled upon an instruction to join outside of discovery loop, may as well autojoin the device
                                TRACE_OUTS("openThings_receive(): New device found, sending ACK: deviceId:");
                                TRACE_OUTN(iDeviceId);
                                TRACE_NL();
                                joining = true;
                                openThings_joinACK(productId, iDeviceId, 20);
                                break;
                            case OTP_TEMPERATURE: // TEMPERATURE
                                // Seems that TEMPERATURE (OTP_TEMPERATURE) received as type OTR_INT=1, and it should be OTR_FLOAT=2 from the eTRV, so override and return a float instead
                                sprintf(OTrecord, ",\"%s\":%.1f", OTrecs[i].paramName, OTrecs[i].retFloat);
                                break;
                            }
                            break;
                        case OTR_FLOAT:
                            sprintf(OTrecord, ",\"%s\":%f", OTrecs[i].paramName, OTrecs[i].retFloat);
                        }

                        //add OT record to returned msg
                        strcat(OTmsg, OTrecord);
                    }

                    // Add to deviceList
                    OTdi = openThings_devicePut(iDeviceId, mfrId, productId, joining);

                    // Update eTRV data and append stored info, only one record is ever returned
                    if (productId == PRODUCTID_MIHO013)
                    {
                        eTRV_update(OTdi, OTrecs[0], rxMsg.t);

                        //if (OTrecs[0].paramId == OTP_TEMPERATURE || )
                        //{
                            // Add static params to returned message
                            eTRV_get_status(OTdi, OTmsg, buflen);
                        //}
                    }

                    // close record array
                    strcat(OTmsg, "}");

                    TRACE_OUTS("openThings_receive: Returning: ");
                    TRACE_OUTS(OTmsg);
                    TRACE_NL();

                    // we have a message, return
                    return records;
                }
                else
                {
                    // Message read from the buffer was not a valid OpenThings message, loop immediately to get the next msg from buffer
                }
            }
            else
            {
                // no messages remaining in the buffer
                //bufferEmpty = true;
            }

        } while (msgsInRxBuf > 0); // loop until the RxMsg buffer is empty

        if (timeout > 0)
        {
            // Rx buffer is empty, sleep a bit before emptying again
            gettimeofday(&currentTime, NULL);
            timersub(&currentTime, &startTime, &diffTime);
            diff = (diffTime.tv_sec * 1000) + diffTime.tv_usec;

            // sleep a very small bit if we are in WaitForMsg mode for eTRVs only, these have an Rx window of 200ms
            if (diff < timeout)
            {
                if (g_CachedCmds > 0)
                {
                    usleep(50000); // 50ms
                }
                else
                {
                    usleep(5000000); // 5s
                }
            }
        }

    } while (timeout > diff);

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
    int i;
    char deviceStr[100];

    TRACE_OUTS("openthings_deviceList(): called\n");

    if (g_NumDevices == 0 || scan)
    {
        // If we dont have any learnt devices yet, or a scan is being forced
        openthings_scan(11);
    }

    sprintf(devices, "{\"g_numDevices\":%d, \"devices\":[\n", g_NumDevices);

    for (i = 0; i < g_NumDevices; i++)
    {
        // add device to JSON
        sprintf(deviceStr, "{\"mfrId\":%d,\"productId\":%d,\"deviceId\":%d,\"control\":%d,\"product\":\"%s\",\"joined\":%d}",
                g_OTdevices[i].mfrId, g_OTdevices[i].productId, g_OTdevices[i].deviceId, g_OTdevices[i].control, g_OTdevices[i].product, g_OTdevices[i].joined);
        strcat(devices, deviceStr);
        if (i + 1 < g_NumDevices)
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

    return g_NumDevices;
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
    int records, i, j;
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
            // message available
            records = openThings_decode(rxMsg.msg, &mfrId, &productId, &iDeviceId, OTrecs);

            if (records > 0)
            {
                joining = false;

                // scan records for JOIN requests, and reply to add
                for (j = 0; j < records; j++)
                {
                    if (OTrecs[i].paramId == OTP_JOIN)
                    {
                        TRACE_OUTS("openThings_scan(): New device found, sending ACK: deviceId:");
                        TRACE_OUTN(iDeviceId);
                        TRACE_NL();
                        joining = true;
                        openThings_joinACK(productId, iDeviceId, 20);
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
** The OpenThings messages are comprised of 3 parts:
**  Header  - msgLength, manufacturerId, productId, encryptionPIP, and deviceId
**  Records - The body of the message, in this case a single command to switch the state
**  Footer  - CRC
**
** Functions performed include:
**    encoding of the device and join request
**    formatting and encoding the OpenThings FSK radio request
**    sending the radio request via the ENER314-RT RaspberryPi adaptor
**
** NOTE: There is an extremely small chance we could lose an incoming message here, but as we are adding new devices it's not worth bothering
**
*/
unsigned char openThings_joinACK(unsigned char iProductId, unsigned int iDeviceId, unsigned char xmits)
{
    int ret = 0;
    unsigned short crc;
    unsigned char radio_msg[OTA_MSGLEN] = {OTA_MSGLEN - 1, ENERGENIE_MFRID, PRODUCTID_MIHO005, OT_DEFAULT_PIP, OT_DEFAULT_DEVICEID, OTC_JOIN_ACK, 0x00, 0x00};

    //printf("openThings_joinACK(): productId=%d, deviceId=%d\n", iProductId, iDeviceId);

    /*
    ** Stage 1: Build the message to send
    */

    // TODO: remove this, and use build_msg instead

    /* Stage 1a: OpenThings HEADER
    */
    radio_msg[OTH_INDEX_PRODUCTID] = iProductId;
    // deviceId
    radio_msg[OTH_INDEX_DEVICEID] = (iDeviceId >> 16) & 0xFF;    //MSB
    radio_msg[OTH_INDEX_DEVICEID + 1] = (iDeviceId >> 8) & 0xFF; //MID
    radio_msg[OTH_INDEX_DEVICEID + 2] = iDeviceId & 0xFF;        //LSB

    /*
    ** Stage 1b: OpenThings RECORDS (Commands) - Not required, as ACK record is always the same 
    */

    /*
    ** Stage 1c: OpenThings FOOTER (CRC)
    */
    crc = calculateCRC(&radio_msg[5], (OTA_MSGLEN - 7));
    radio_msg[OTA_MSGLEN - 2] = ((crc >> 8) & 0xFF); // MSB
    radio_msg[OTA_MSGLEN - 1] = (crc & 0xFF);        // LSB

#if defined(TRACE)
    TRACE_OUTS("ACK tx payload (unencrypted):\n");
    for (int i = 0; i < OTA_MSGLEN; i++)
    {
        TRACE_OUTN(radio_msg[i]);
        TRACE_OUTC(',');
    }
    TRACE_NL();
#endif

    // Stage 1d: encrypt body part of message (default PIP is OK here)
    cryptMsg(CRYPT_PID, CRYPT_PIP, &radio_msg[5], (OTA_MSGLEN - 5));

    // mutex access radio adaptor
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

/*
** openThings_cache_send()
** ===================
** Send any cached command to an eTRV OpenThings based Energenie smart device
** This is designed for devices that have a small receive window such as the 'MiHome Heating' TRV
**
** set cached command using openThings_cmd()
*/
int openThings_cache_send(unsigned int iDeviceId)
{
    int index = 0;
    unsigned char msglen;

    /*
    ** The full command is cached in the g_OTdevices trv array
    */

    // check the global first (quick) to see if we have any cached cmds outstanding
    if (g_CachedCmds > 0)
    {
        index = openThings_getDeviceIndex(iDeviceId);
        if (index >= 0)
        {
            // first check if we have outstanding cached commands; these take precedence
            if (g_OTdevices[index].trv->retries > 0)
            {
                msglen = g_OTdevices[index].trv->cachedCmd[0] + 1; // msglen in radio message doesn't include the length byte :)
                if (msglen > 1)
                {
                    // we have a cached command, send it
                    if ((lock_ener314rt()) == 0)
                    {
                        radio_mod_transmit(RADIO_MODULATION_FSK, g_OTdevices[index].trv->cachedCmd, msglen, 1); //TODO make xmits configurable

                        unlock_ener314rt();
                        g_OTdevices[index].trv->retries--;
#if defined(TRACE)
                        printf("openThings_cache_send(): g_CachedCmds=%d, deviceId=%d, retries=%d\n", g_CachedCmds, iDeviceId, g_OTdevices[index].trv->retries);
#endif
                    }
                    else
                    {
                        // critical error, cannot get lock
                        return -2;
                    }
                }
                else
                {
                    // no message to send!
                    return 0;
                }
            }
            else
            {
                // No outstanding cached commands
                // TO-DO periodic auto-reporting
                return 0;
            }
        } // else unknown device
    }
    return index;
}

/*
** eTRV_update()
** ===================
** Store Rx record data in the eTRV record structure for reporting
**
**  OTdi - Index in g_OTdevices array (for speed)
**  OTrecord - The record received
*/
void eTRV_update(int OTdi, struct OTrecord OTrec, time_t updateTime)
{
    struct TRV_DEVICE *trvData;
    trvData = g_OTdevices[OTdi].trv; // make a pointer to correct struct in array for speed

    switch (OTrec.paramId)
    {
    case OTP_TEMPERATURE:
        trvData->currentC = OTrec.retFloat;
        break;
    case OTP_VOLTAGE:
        trvData->voltage = OTrec.retFloat;
        trvData->voltageDate = updateTime;

        // Do we need to clear cached cmd retries?
        if (trvData->command == OTCP_REQUEST_VOLTAGE)
        {
            trvData->retries = 0;
            g_CachedCmds--;
        }
        break;
    case OTP_DIAGNOSTICS:
        trvData->diagnostics = OTrec.retInt;
        trvData->diagnosticDate = updateTime;
        trvData->errors = false; // clear errors, will set again below
        trvData->errString[0] = '\0';

        // Do we need to clear cached cmd retries? (Exercise valve cmd returns diags too!)
        if (trvData->command == OTCP_REQUEST_DIAGNOTICS || trvData->command == OTCP_EXERCISE_VALVE)
        {
            trvData->retries = 0;
            g_CachedCmds--;
        }

        // Is there any specific diag data we need to store as well?
        if (OTrec.retInt > 0)
        {
            // we have diagnostic flags
            if (OTrec.retInt & 0x0001)
            { // Motor current below expectation
                trvData->errors = true;
                strncpy(trvData->errString, "Motor current below expectation.", MAX_ERRSTR);
            }
            if (OTrec.retInt & 0x0002)
            { // Motor current always high
                trvData->errors = true;
                strncat(trvData->errString, "Motor current always high.", MAX_ERRSTR);
            }
            if (OTrec.retInt & 0x0004)
            { // Motor taking too long
                trvData->errors = true;
                strncat(trvData->errString, "Motor taking too long to open/close.", MAX_ERRSTR);
            }
            if (OTrec.retInt & 0x0008)
            { // Discrepancy between air and pipe sensors
                strncat(trvData->errString, "Discrepancy between air and pipe sensors.", MAX_ERRSTR);
            }
            if (OTrec.retInt & 0x0010)
            { // Air sensor out of expected range
                trvData->errors = true;
                strncat(trvData->errString, "Air sensor out of expected range.", MAX_ERRSTR);
            }
            if (OTrec.retInt & 0x0020)
            { // Pipe sensor out of expected range
                trvData->errors = true;
                strncat(trvData->errString, "Pipe sensor out of expected range.", MAX_ERRSTR);
            }
            if (OTrec.retInt & 0x0040)
            { // LOW_POWER_MODE
                trvData->lowPowerMode = true;
            }
            else
            {
                trvData->lowPowerMode = false;
            }
            if (OTrec.retInt & 0x0080)
            { // No target temperature has been set by host
                trvData->targetC = 0;
            }
            if (OTrec.retInt & 0x0100)
            { // Valve may be sticking
                trvData->valve = ERROR;
                trvData->errors = true;
            }
            if (OTrec.retInt & 0x0200)
            { // EXERCISE_VALVE success
                trvData->exerciseValve = true;
                trvData->valveDate = updateTime;
            }
            if (OTrec.retInt & 0x0400)
            { // EXERCISE_VALVE fail
                trvData->exerciseValve = false;
                trvData->valveDate = updateTime;
                trvData->errors = true;
            }
            if (OTrec.retInt & 0x0800)
            { // Driver micro has suffered a watchdog reset and needs data refresh
                trvData->errors = true;
                strncat(trvData->errString, "Driver micro has suffered a watchdog reset and needs data refresh.", MAX_ERRSTR);
            }
            if (OTrec.retInt & 0x1000)
            { // Driver micro has suffered a noise reset and needs data refresh
                trvData->errors = true;
                strncat(trvData->errString, "Driver micro has suffered a noise reset and needs data refresh.", MAX_ERRSTR);
            }
            if (OTrec.retInt & 0x2000)
            { // Battery voltage has fallen below 2p2V and valve has been opened
                trvData->errors = true;
                strncat(trvData->errString, "Battery voltage has fallen below 2.2V and valve has been opened.", MAX_ERRSTR);
            }
            if (OTrec.retInt & 0x4000)
            { // Request for heat messaging is enabled - not sure what to do here, or even how to set this!
                //trvData->
            }
            if (OTrec.retInt & 0x8000)
            { // Request for heat  - not sure what to do here
                //trvData->
            }
        }
        else
        {
            // some flags may need clearing as we have 0
            trvData->lowPowerMode = false;
        }
    }
}

/*
** eTRV_get_status()
** ===================
** JSONify stored data for the eTRV record structure for reporting
** data is appended to incoming buf as key value comma separated pairs
**
**  OTdi - Index in g_OTdevices array (for speed)
**  buf  - buf appended with new data
**  buflen - length of buffer to prevent memory errors
*/
void eTRV_get_status(int OTdi, char *buf, unsigned int buflen)
{
    struct TRV_DEVICE *trvData;
    trvData = g_OTdevices[OTdi].trv; // make a pointer to correct struct in array for speed
    char trvStatus[200] = "";
    static const char *VALVE_STR[] = {"open", "closed", "auto", "error", "unknown"};

    // populate outstanding command
    if (trvData->retries > 0)
    {
        sprintf(trvStatus, ",\"command\":%d,\"retries\":%d",
                trvData->command,
                trvData->retries);
        strncat(buf, trvStatus, buflen);
    }
    if (trvData->targetC > 0)
    {
        sprintf(trvStatus, ",\"TARGET_C\":%.1f", trvData->targetC);
        strncat(buf, trvStatus, buflen);
    }
    if (trvData->voltage > 0)
    {
        sprintf(trvStatus, ",\"VOLTAGE\":%.2f,\"VOLTAGE_TS\":%d", trvData->voltage, (int)trvData->voltageDate);
        strncat(buf, trvStatus, buflen);
    }
    if (trvData->valve != UNKNOWN)
    {
        sprintf(trvStatus, ",\"VALVE_STATE\":\"%s\"", VALVE_STR[trvData->valve]);
        strncat(buf, trvStatus, buflen);
    }
    if (trvData->valveDate > 0)
    {
        sprintf(trvStatus, ",\"EXERCISE_VALVE\":\"%s\",\"VALVE_TS\":%d",
                trvData->exerciseValve ? "success" : "fail",
                (int)trvData->valveDate);
        strncat(buf, trvStatus, buflen);
    }
    if (trvData->diagnosticDate > 0)
    {
        sprintf(trvStatus, ",\"DIAGNOSTICS\":%d,\"DIAGNOSTICS_TS\":%d,\"LOW_POWER_MODE\":%s",
                trvData->diagnostics,
                (int)trvData->diagnosticDate,
                trvData->lowPowerMode ? "true" : "false");
        strncat(buf, trvStatus, buflen);
    }
    if (trvData->errors)
    {
        sprintf(trvStatus, ",\"ERRORS\":%s,\"ERROR_TEXT\":\"%s\"",
                trvData->errors ? "true" : "false",
                trvData->errString);
        strncat(buf, trvStatus, buflen);
    }

#if defined(TRACE)
    printf("eTRV_get_status(): %s, strlen=%d buflen:%d\n",trvStatus,strlen(trvStatus),buflen);
#endif
}