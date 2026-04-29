/**
 * Modbus.h - Modbus 通讯头文件
 */

#ifndef __MODBUS_H
#define __MODBUS_H

#include "stdint.h"

void Modbus_Init(void);
void Modbus_CreateTask(void);
void CallBack_FromUsart1(uint8_t *p, uint16_t length, uint8_t Usart_Sel);

#endif
