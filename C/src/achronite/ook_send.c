#include <stdio.h>
#include <stdbool.h>
#include "ook_send.h"
#include "lock_radio.h"
#include "openThings.h"
#include "../energenie/radio.h"
#include "../energenie/trace.h"

/*
** C module addition to energenie code to simplify the sending of OOK based transmissions to the Energenie ENER314-RT
** by only needing 1 call to C.
**
** Author: Phil Grainger - @Achronite, January 2019
*/

/* encodeDecimal()
** encodes any 'iDecimal' value into a transmittable byte array 'encArray' of length 'bits' compatible with the energenie OOK radio protocol
** this is primarily used to encode the house code
*/
void encodeDecimal(unsigned int iDecimal, unsigned char bits, unsigned char *encArray)
{
    int c, msb, lsb, iByte = 0;
    unsigned char encByte;

    for (c = bits - 1; c >= 0; c -= 2)
    {
        msb = iDecimal >> c;
        lsb = iDecimal >> (c - 1);

        if (msb & 1)
        {
            // Assuming 0 for LSB 1110:1000 = 232
            encByte = 232;
        }
        else
        {
            // Assuming 0 for LSB 1000:1000 = 136
            encByte = 136;
        }

        if (lsb & 1)
        {
            // LSB is 1 so add x11x = 6 to MSB
            encByte += 6;
        }

        /* add to encoded byte array */
        encArray[iByte++] = encByte;
    }
}

/*
** OokSend
** =======
** Send a switch signal to a 'Control only' RF OOK based Energenie smart switch adaptors, sockets, switches and relays
** Currently this covers all smart switchable devices except the 'HiHome Adaptor Plus' and 'MiHome Heating' TRV
**
** Functions performed include:
**    initialising the radio and setting the modulation
**    encoding of the house code/zone and switch request
**    formatting and encoding an OOK radio request
**    sending the radio request via the ENER314-RT RaspberryPi adaptor
*/
unsigned char OokSend(unsigned int iZone, unsigned char iSwitchNum, unsigned char bSwitchState, unsigned char xmits)
{
    int ret = 0;
    unsigned char radio_msg[OOK_MSGLEN] = {PREAMBLE, DEFAULT_HC, 0x00, 0x00};

    printf("ookSend: Zone=%d, Switch=%d, state=%d\n", iZone, iSwitchNum, bSwitchState);

    // encode the zone / house code if not using the default
    if (iZone != USE_DEFAULT_ZONE)
    {
        encodeDecimal(iZone, ZONE_BITS, &radio_msg[INDEX_HC]);
    }

    /* Encode the 2 byte switch code for OFF, minimising calculations */
    switch (iSwitchNum)
    {
    case 0: // switch all in zone
        radio_msg[INDEX_SC] = 238;
        radio_msg[INDEX_SC + 1] = 136;
        break;
    case 1:
        radio_msg[INDEX_SC] = 238;
        radio_msg[INDEX_SC + 1] = 232;
        break;
    case 2:
        radio_msg[INDEX_SC] = 142;
        radio_msg[INDEX_SC + 1] = 232;
        break;
    case 3:
        radio_msg[INDEX_SC] = 232;
        radio_msg[INDEX_SC + 1] = 232;
        break;
    case 4:
        radio_msg[INDEX_SC] = 136;
        radio_msg[INDEX_SC + 1] = 232;
        break;
    case 5: // undocumented
        radio_msg[INDEX_SC] = 142;
        radio_msg[INDEX_SC + 1] = 136;
        break;
    case 6: // undocumented
        radio_msg[INDEX_SC] = 232;
        radio_msg[INDEX_SC + 1] = 136;
        break;
    default:
        // switch out of range, return error code
        return -1;
    }

    if (bSwitchState)
    {
        // State bit (d3) is encoded as x11x = 6 when on, so add 6 for ON state (as proceeding x bits are already encoded)
        radio_msg[INDEX_SC + 1] += 6;
    }

    // lock adaptor
    if ((ret = lock_ener314rt()) == 0)
    {
        /*
        ** flush Rx buffer if required
        */
        ret = empty_radio_Rx_buffer(DT_CONTROL);
        printf("ookSend(%d): Rx buffer", ret);

        // Transmit OOK encoded payload 26ms per payload * xmits
        radio_mod_transmit(RADIO_MODULATION_OOK, radio_msg, OOK_MSGLEN, xmits);

        //unlock adaptor
        unlock_ener314rt();

        // place radio into standby mode, this may need to change to support FSK or receive mode
        //radio_standby();
    };

    return ret;
}