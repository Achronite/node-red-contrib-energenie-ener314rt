#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
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
** cryptByte()- en/decrypt a single Byte of an OpenThings message
** Code converted from python module by @whaleygeek
*/
unsigned char cryptByte(unsigned char data)
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
** cryptMsg() - en/decrypt an OpenThings message (destructive)
**
** Code converted from python module by @whaleygeek
*/
void cryptMsg(unsigned char pid, unsigned short pip, unsigned char *msg, unsigned int length)
{
    unsigned char i;

    ran = (((pid&0xFF)<<8) ^ pip);
    for (i=0; i<length; i++){
        msg[i] = cryptByte(msg[i]);
    }
}

/*
** OTtypelen() - return the number of bits used to encode a specific OpenThings record data type
*/
char OTtypelen(unsigned char OTtype){
    char bits = 0;

    switch(OTtype){
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

/* convertBytes() - Convert a byte array into a given OpenThings single value
**
** uint8_t:  unsigned char
** uint16_t: unsigned short
** uint32_t: unsigned int
** uint64_t: unsigned long long
*/

/*
** openThings_decode()
** =======
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
    float f;

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
        *mfrId      = payload[1];
        *productId  = payload[2];
        pip = (payload[3]<<8) + payload[4];

        //struct sOTHeader = { length, mfrId, productId, pip };
        
        // decode body
        cryptMsg(CRYPT_PID, pip, &payload[5], (length-4));
        *iDeviceId = (payload[5]<<16) + (payload[6]<<8) + payload[7];

        // CHECK CRC from last 2 bytes of message
	    crca  = (payload[length-1]<<8) + payload[length];
        crc = calculateCRC(&payload[5], (length-6));


	    if (crc != crca) {
            // CRC does not match
	        printf("openThings_decode(): ERROR crc actual:%d expected:%d\n", crca, crc);
            return -2;
        } else {
            // CRC OK
            
            //DECODE RECORDS
            i = 8;  // start at the 1st record
	        printf("openThings_decode(): length:%d\n", length);

            while ((i < length) && (payload[i] != 0) && (record < OT_MAX_RECS)){
                // reset values
                memset(recs[record].retChar, '\0', 15);
                result = 0;
                
                // PARAM
                param = payload[i++];
                recs[record].wr = ((param & 0x80) == 0x80);
                recs[record].paramId = param & 0x7F;

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
                printf("openThings_decode(): record[%d] paramid=0x%02x typeId=0x%02x values:[", record, recs[record].paramId, recs[record].typeId);

                // In C, it is not great at returning different types for a function; so we are just going to have to code it here rather than be a modular coder :(
                switch(recs[record].typeId)
                {
                    case OT_CHAR:
                        for (j=0; j<rlen; j++)
                        {
                            printf("%d,",payload[i+j]);
                            recs[record].retChar[j] = payload[i+j];
                        }
                        recs[record].typeIndex = 1;
                        break;
                    case OT_UINT:
                    case OT_UINT4:
                    case OT_UINT8:
                    case OT_UINT12:
                    case OT_UINT16:
                    case OT_UINT20:
                    case OT_UINT24:
                        for (j=0; j<rlen; j++)
                        {
                            printf("%d,",payload[i+j]);
                            result <<= 8;
                            result += payload[i+j];
                        }
                        recs[record].typeIndex = 2;
                        break;
                    case OT_SINT:
                    case OT_SINT8:
                    case OT_SINT16:
                    case OT_SINT24:
                        for (j=0; j<rlen; j++)
                        {
                            printf("%d,",payload[i+j]);
                            result <<= 8;
                            result += payload[i+j];
                        }                        // turn to signed int based on high bit of MSB, 2's comp is 1's comp plus 1
                        if ((payload[i] & 0x80) == 0x80) {
                            // negative
                            result = -( ((!result) & ((2^(length*8) )-1) ) + 1);
                            //onescomp = (~result) & ((2**(length*8))-1)
				            //result = -(onescomp + 1)
                            // =result = -((~result) & ((2**(length*8))-1) + 1)
                        }
                        recs[record].typeIndex = 3;
                        break;
                    case OT_FLOAT:
                        // TODO (@whaleygeek didnt do this either!)
                        recs[record].typeIndex = -1;
                        break;
                    default:
                        // TODO - are there other values?
                        recs[record].typeIndex = -2;
                }

                // TODO: Binary point adjustment, which could be tricky returning a float value
                // 			return (float(result)) / (2**Value.typebits(typeid))
                f = (float)result / (2 ^ OTtypelen(recs[record].typeId));
                // always store the integer result in the record
                recs[record].retInt = result;

                printf("] typeIndex:%d Int:%d Float:%f Char:%s\n", recs[record].typeIndex, recs[record].retInt, f, recs[record].retChar);

                // move arrays on
                i+=rlen;
                record++;
            }
        }
        
    //}

    // return the number of records
    return record;

}
/* PYTHON
def decode(payload, decrypt=True, receive_timestamp=None):
	"""Decode a raw buffer into an OpenThings pydict"""
	#Note, decrypt must already have run on this for it to work
	length = payload[0]

	# CHECK LENGTH
	if length+1 != len(payload) or length < 10:
		raise OpenThingsException("bad payload length")
		##return {
		##	"type":         "BADLEN",
		##	"len_actual":   len(payload),
		##	"len_expected": length,
		##	"payload":      payload[1:]
		##}

	# DECODE HEADER
	mfrId      = payload[1]
	productId  = payload[2]
	encryptPIP = (payload[3]<<8) + payload[4]
	header = {
		"mfrid"     : mfrId,
		"productid" : productId,
		"encryptPIP": encryptPIP
	}


	if decrypt:
		# DECRYPT PAYLOAD
		# [0]len,mfrid,productid,pipH,pipL,[5]
		crypto.init(crypt_pid, encryptPIP)
		crypto.cryptPayload(payload, 5, len(payload)-5) # including CRC
		##printhex(payload)
	# sensorId is in encrypted region
	sensorId = (payload[5]<<16) + (payload[6]<<8) + payload[7]
	header["sensorid"] = sensorId


	# CHECK CRC
	crc_actual  = (payload[-2]<<8) + payload[-1]
	crc_expected = calcCRC(payload, 5, len(payload)-(5+2))
	##trace("crc actual:%s, expected:%s" %(hex(crc_actual), hex(crc_expected)))

	if crc_actual != crc_expected:
		raise OpenThingsException("bad CRC")
		##return {
		##	"type":         "BADCRC",
		##	"crc_actual":   crc_actual,
		##	"crc_expected": crc_expected,
		##	"payload":      payload[1:],
		##}


	# DECODE RECORDS
	i = 8
	recs = []
	while i < length and payload[i] != 0:
		# PARAM
		param = payload[i]
		wr = ((param & 0x80) == 0x80)
		paramid = param & 0x7F
		if paramid in param_info:
			paramname = (param_info[paramid])["n"] # name
			paramunit = (param_info[paramid])["u"] # unit
		else:
			paramname = "UNKNOWN_" + hex(paramid)
			paramunit = "UNKNOWN_UNIT"
		i += 1

		# TYPE/LEN
		typeid = payload[i] & 0xF0
		plen = payload[i] & 0x0F
		i += 1

		rec = {
			"wr":         wr,
			"paramid":    paramid,
			"paramname":  paramname,
			"paramunit":  paramunit,
			"typeid":     typeid,
			"length":     plen
		}

		if plen != 0:
			# VALUE
			valuebytes = []
			for x in range(plen):
				valuebytes.append(payload[i])
				i += 1
			value = Value.decode(valuebytes, typeid, plen)
			rec["valuebytes"] = valuebytes
			rec["value"] = value

		# store rec
		recs.append(rec)

	m = {
		"type":    "OK",
		"header":  header,
		"recs":    recs
	}
	if receive_timestamp != None:
		m["rxtimestamp"] = receive_timestamp
	return Message(m)
*/


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
    cryptMsg(CRYPT_PID, CRYPT_PIP, &radio_msg[5], (OTS_MSGLEN-5));

    /*
    ** Transmit via radio adaptor, using mutex to block the radio
    */

    // mutex access radio adaptor (for a while!)
    pthread_mutex_lock(&radio_mutex);

    // Set FSK mode for OpenThings devices
    radio_modulation(RADIO_MODULATION_FSK);

    // Transmit encoded payload 26ms per payload * xmits
    radio_transmit(radio_msg,OTS_MSGLEN,xmits);

    // release mutex lock
    pthread_mutex_unlock(&radio_mutex);

    // place radio into standby mode, this may need to change to support receive mode
    //radio_standby();
  
    return ret;
}
/*
** openThings_discover()
** =======
** Discover 'Monitor' RF FSK OpenThings based Energenie smart devices
** Currently this covers the 'HiHome Adaptor Plus', 'MiHome Heating' TRV, ++++
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
**    returning array of discovered devices as [productId:deviceId]
*/
unsigned char openThings_discover(unsigned char iTimeOut, char *devices )
{
    int ret = 0;
    uint8_t buf[MAX_FIFO_BUFFER];
    struct OTrecord OTrecs[OT_MAX_RECS];
    unsigned char mfrId, productId;
    unsigned int iDeviceId;
    int records, i;
    char deviceStr[50];

    printf("openthings_discover(): called\n");

    /* check if radio initialised */


/* PYTHON
    radio.receiver(fsk=True)
    timeout = time.time() + receive_time
    handled = False

    while True:
        if radio.is_receive_waiting():
            payload = radio.receive_cbp()
            now = time.time()
            try:
                msg        = OpenThings.decode(payload, receive_timestamp=now)
                hdr        = msg["header"]
                mfr_id     = hdr["mfrid"]
                product_id = hdr["productid"]
                device_id  = hdr["sensorid"]
                address    = (mfr_id, product_id, device_id)

                registry.fsk_router.incoming_message(address, msg)
                handled = True
            except OpenThings.OpenThingsException:
                print("Can't decode payload:%s" % payload)

        now = time.time()
        if now > timeout: break

    return handled
*/
    // reset devices response JSON
    strcpy(devices,"{\"devices\":[");

    // mutex access radio adaptor (for a while!)
    pthread_mutex_lock(&radio_mutex);

    // Set FSK mode receive for OpenThings devices
    radio_receiver(RADIO_MODULATION_FSK);

    // Loop until timeout
    do {
        sleep(1); // pause 1 second
        if ( radio_is_receive_waiting() ){
            //printf("openthings_discover(): radio_is_receive waiting\n");
            if ( radio_get_payload_cbp(buf, MAX_FIFO_BUFFER) == RADIO_RESULT_OK){
                // Received a valid payload, decode it
                records = openThings_decode(buf, &mfrId, &productId, &iDeviceId, OTrecs);
                if (records > 0){
                    printf("Valid OpenThings Message. mfrId:%d productId:%d deviceId:%d records:%d\n", mfrId, productId, iDeviceId, records);

                    // Add to discovered devices
                    sprintf(deviceStr, "{\"mfrId\":%d,\"productId\":%d,\"deviceId\":%d}", mfrId, productId, iDeviceId);
                    strcat(devices, deviceStr);

                    // add 1 to devices discovered
                    ret++;

                    // print records for now, we will need them later when we implement receive mode
                    /*
                    for (i=0; i<records; i++){
                        printf("record %d: [wr:%d, paramId:0x%02x, typeId:0x%02x, typeIndex:%d, integer:%d, chars:%s]\n",
                            i,
                            OTrecs[i].wr,
                            OTrecs[i].paramId,
                            OTrecs[i].typeId,
                            OTrecs[i].typeIndex,
                            OTrecs[i].retInt,
                            OTrecs[i].retChar);
                    }
                    */

                } else {
                    printf("Invalid OT Payload, return=%d\n",records);
                }
            }
        } else {
            //printf("openthings_discover(%d): radio_is_receive waiting=FALSE\n",iTimeOut);
        }
    } while (iTimeOut-- > 0);

    //unlock mutex
    pthread_mutex_unlock(&radio_mutex);
   
    // Close JSON array
    sprintf(deviceStr, "],\"numDevices\":%d}", ret);
    strcat(devices, deviceStr);
    printf("Returning:\n%s\n",devices);

    //radio_standby();

    return ret;
}