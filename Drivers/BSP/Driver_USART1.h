

#ifndef _DRIVER_USART_1_H_
#define _DRIVER_USART_1_H_


#include "stdint.h"
#include "stdbool.h"

void	USART1_Send(uint8_t *p, uint16_t Length, bool isWaiting);
void	USART1_Init(void);







#endif


