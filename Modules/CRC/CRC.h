/**
 * CRC.h - CRC16 Modbus 头文件
 */

#ifndef __CRC_H
#define __CRC_H

#include "stdint.h"

uint16_t Modbus_CRC16_Lookup(uint8_t *data, uint16_t length);
uint16_t CRC16_Modbus(uint8_t *data, uint16_t length);

#endif
