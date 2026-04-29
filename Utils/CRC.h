


#ifndef _CRC_H_
#define _CRC_H_

#include "stdint.h"

uint8_t 	CRC4_TableMode(uint8_t *p, uint32_t length);

uint8_t 	CRC4(uint8_t *p, uint32_t length);
uint8_t		CRC4_1(uint8_t *p, uint32_t length);

void 			make_crc_table( void );
uint32_t 	crc4_lookup_fast(uint32_t p);

uint16_t 	Modbus_CRC16_Lookup(uint8_t *pFram,uint16_t len);

#endif
