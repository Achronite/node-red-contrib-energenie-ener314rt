/* radio.h  03/04/2016  D.J.Whale
 *
 * Energenie radio interface
 */


#ifndef _RADIO_H
#define _RADIO_H

#include "system.h"

typedef uint8_t RADIO_RESULT;
#define RADIO_RESULT_IS_ERR(R)         (((R) & 0x80) != 0)
#define RADIO_RESULT_OK                0x00
#define RADIO_RESULT_OK_FALSE          0x00
#define RADIO_RESULT_OK_TRUE           0x01
#define RADIO_RESULT_ERR_UNIMPLEMENTED 0x80
#define RADIO_RESULT_ERR_LONG_PAYLOAD  0x81
#define RADIO_RESULT_ERR_READ_FAILED   0x82

typedef uint8_t RADIO_MODULATION;
#define RADIO_MODULATION_OOK 0
#define RADIO_MODULATION_FSK 1

typedef uint8_t RADIO_MODE;

//extern void radio_init(void);
void radio_reset(void);
int radio_init(void);
uint8_t radio_get_ver(void);
void radio_modulation(RADIO_MODULATION mod);
void radio_transmitter(RADIO_MODULATION mod);
void radio_receiver(RADIO_MODULATION mod);
void radio_standby(void);
void radio_transmit(uint8_t* payload, uint8_t len, uint8_t times);
void radio_send_payload(uint8_t* payload, uint8_t len, uint8_t times);
RADIO_RESULT radio_is_receive_waiting(void);
RADIO_RESULT radio_get_payload_len(uint8_t* buf, uint8_t buflen);
RADIO_RESULT radio_get_payload_cbp(uint8_t* buf, uint8_t buflen);
void radio_finished(void);
void radio_setmode(RADIO_MODULATION mod, RADIO_MODE mode);
void radio_mod_transmit(RADIO_MODULATION mod, uint8_t* payload, uint8_t len, uint8_t times);

#endif

/***** END OF FILE *****/
