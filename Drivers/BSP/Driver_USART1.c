#include "USART_RegDefine.h"
#include "stm32f10x_conf.h"
#include "stdint.h"
#include "Driver_USART1.h"
#include "Usart_Define.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"
#include "stdbool.h"

#define		USE_DMA   								0
#define		UART_COMPLTET_IDLE				1


#define		USART_Tx_Port							GPIOA
#define		USART_Tx_Pin							GPIO_Pin_9

#define		USART_Rx_Port							GPIOA
#define		USART_Rx_Pin							GPIO_Pin_10

#define		USART_Drv									USART1
#define		USART_Enable_Clock()			RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE)

#define		USART_IRQn								USART1_IRQn
#define		USART_ISR									USART1_IRQHandler

#if USE_DMA
#define		DMA_Drv										DMA1
#define		DMA_USART_TxChannel				DMA1_Channel4
#define		DMA_USART_TxChannel_IRQn	DMA1_Channel4_IRQn
#define		DMA_USART_RxChannel				DMA1_Channel5
#define		DMA_USART_RxChannel_IRQn	DMA1_Channel5_IRQn

#define		DMA_Enable_Clock()				RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE)
#define		DMA_USART_RX_IRQHandler		DMA1_Channel5_IRQHandler
#define		DMA_USART_TX_IRQHandler		DMA1_Channel4_IRQHandler
#define		DMA_USART_RX_IT_TC				DMA1_IT_TC5
#define		DMA_USART_RX_IT_TE				DMA1_IT_TE5
#define		DMA_USART_TX_IT_TC				DMA1_IT_TC4
#define		DMA_USART_TX_IT_TE				DMA1_IT_TE4

static DMA_CCR_Reg_t * const pDMA_TxCh_CCR 	= (DMA_CCR_Reg_t*)&DMA_USART_TxChannel->CCR;
//DMA_CCR_Reg_t * const pDMA_RxCh_CCR 	= (DMA_CCR_Reg_t*)&DMA_USART_RxChannel->CCR;
#endif

static void USART_Timer_CallBack(TimerHandle_t xTimer);
static TimerHandle_t timid_USART;

static void USART_Thread1(void *arg);
static TaskHandle_t tid_USART;

volatile static	uint8_t	Event 				= 0;


#define									RxBufLength			256

static uint8_t					RxBuf1[RxBufLength],RxBuf2[RxBufLength];
static uint16_t				RxCount					= 0;
static uint8_t					*pRxBuf					= RxBuf1;
static bool						isDMA_Busy			= false;

#if !USE_DMA
static uint8_t					*pTxBuf					= false;
static uint16_t				TxCount					= 0;
static uint16_t				TxLen						= 0;
#endif

#define						USART_DefBaud		(uint32_t)115200
static uint32_t	USART_BaudRate				= 115200;
#if USE_DMA
static void			USART_InitDMA(void);
#endif

//void	CallBack_FromUsart1(uint8_t *p, uint16_t length);
void	CallBack_FromUsart1(uint8_t *p, uint16_t length, Usart_Sel_t Usart_Sel);

#if		UART_COMPLTET_IDLE
static bool			isCallBack_Busy	= false;
#endif

static void USART_Timer_CallBack(TimerHandle_t xTimer){
    (void)xTimer;
	xTimerStop(timid_USART, 0);
#if		UART_COMPLTET_IDLE
	if(isCallBack_Busy)
		return;
	isCallBack_Busy		= true;
#endif
	pRxBuf[RxCount]		= '\0';
	CallBack_FromUsart1(pRxBuf, RxCount, Usart_Sel_1);
	RxCount						= 0;
	if(pRxBuf == RxBuf1)
		pRxBuf					= RxBuf2;
	else
		pRxBuf					= RxBuf1;
#if		UART_COMPLTET_IDLE
	isCallBack_Busy		= false;
#endif
}

static void	USART_Thread1(void *arg){
	uint32_t notifiedValue;
	for(;;){
		xTaskNotifyWait(0, 0x55aa, &notifiedValue, portMAX_DELAY);
		if(notifiedValue & 0x55aa){
			if(Event == 1){				//�ֽڽ������
				xTimerChangePeriod(timid_USART, pdMS_TO_TICKS(20), 0);
			}
			else if(Event == 2){	//֡�������
#if		UART_COMPLTET_IDLE
				if(isCallBack_Busy)
					continue;
				isCallBack_Busy		= true;
				xTimerStop(timid_USART, 0);
				pRxBuf[RxCount]		= '\0';
				CallBack_FromUsart1(pRxBuf, RxCount, Usart_Sel_1);
				RxCount						= 0;
				if(pRxBuf == RxBuf1)
					pRxBuf					= RxBuf2;
				else
					pRxBuf					= RxBuf1;
				isCallBack_Busy		= false;
#else
				xTimerChangePeriod(timid_USART, pdMS_TO_TICKS(1), 0);
#endif
			}
			Event		= 0;
		}
	}
}



void	USART1_SetBaud(uint32_t BaudRate){
	while(isDMA_Busy);
	isDMA_Busy					= true;
	USART_InitTypeDef		USART_InitStruct;
	
	
	USART_Cmd(USART_Drv, DISABLE);
	
	USART_InitStruct.USART_BaudRate							= BaudRate;
	USART_InitStruct.USART_HardwareFlowControl	= USART_HardwareFlowControl_None;
	USART_InitStruct.USART_Parity								= USART_Parity_No;
	USART_InitStruct.USART_StopBits							= USART_StopBits_1;
	USART_InitStruct.USART_WordLength						= USART_WordLength_8b;
	USART_InitStruct.USART_Mode									= USART_Mode_Tx | USART_Mode_Rx;
	USART_Init(USART_Drv, &USART_InitStruct);
	
	USART_Cmd(USART_Drv, ENABLE);
	isDMA_Busy					= false;
//	EE_File_WriteFile("Uart1Baud", (uint8_t*)&USART_BaudRate, sizeof(USART_BaudRate));
}




void	USART1_Send(uint8_t *p, uint16_t Length, bool isWaiting){
	int16_t		timeout;
	while(isDMA_Busy);
	if(!p || Length <= 0){
		return;
	}
	isDMA_Busy						= true;
#if USE_DMA
	pDMA_TxCh_CCR->EN			= 0;
	DMA_USART_TxChannel->CMAR	= (uint32_t)p;
	DMA_USART_TxChannel->CNDTR	= Length;
	DMA_USART_TxChannel->CPAR	= (uint32_t)&USART_Drv->DR;
	pDMA_TxCh_CCR->EN			= 1;
	if(isWaiting){
		while(isDMA_Busy);
	}
#else
	USART_ClearFlag(USART_Drv, USART_FLAG_TC);
	if(isWaiting){
		for(uint16_t i = 0;i < Length;i++){
			timeout					= 100;
			USART_Drv->DR		= p[i];
			while(USART_GetFlagStatus(USART_Drv, USART_FLAG_TC) == RESET && timeout++ != 0);
			if(timeout <= 0){
				USART1_Init();
				break;
			}
		}
		isDMA_Busy				= false;
	}
	else{
		pTxBuf						= p;
		TxCount						= 1;
		TxLen							= Length;
		USART_Drv->DR			= p[0];
		USART_ITConfig(USART_Drv, USART_IT_TC, ENABLE);
	}
#endif
}


void	USART1_Init(void){
	GPIO_InitTypeDef		GPIO_InitStruct;
	USART_InitTypeDef		USART_InitStruct;
	NVIC_InitTypeDef		NVIC_InitStruct;
	
	USART_Enable_Clock();
	
	GPIO_InitStruct.GPIO_Mode										= GPIO_Mode_AF_PP;
	GPIO_InitStruct.GPIO_Speed									= GPIO_Speed_10MHz;
	GPIO_InitStruct.GPIO_Pin										= USART_Tx_Pin;
	GPIO_Init(USART_Tx_Port, &GPIO_InitStruct);
	
	GPIO_InitStruct.GPIO_Mode										= GPIO_Mode_IPU;
	GPIO_InitStruct.GPIO_Speed									= GPIO_Speed_10MHz;
	GPIO_InitStruct.GPIO_Pin										= USART_Rx_Pin;
	GPIO_Init(USART_Rx_Port, &GPIO_InitStruct);
	
//	EE_File_ReadFile("Uart1Baud", (uint8_t*)&USART_BaudRate);
	
	USART_InitStruct.USART_BaudRate							= USART_BaudRate;
	USART_InitStruct.USART_HardwareFlowControl	= USART_HardwareFlowControl_None;
	USART_InitStruct.USART_Parity								= USART_Parity_No;
	USART_InitStruct.USART_StopBits							= USART_StopBits_1;
	USART_InitStruct.USART_WordLength						= USART_WordLength_8b;
	USART_InitStruct.USART_Mode									= USART_Mode_Tx | USART_Mode_Rx;
	USART_Init(USART_Drv, &USART_InitStruct);
	
	USART_ITConfig(USART_Drv, USART_IT_RXNE, ENABLE);
	USART_ITConfig(USART_Drv, USART_IT_ORE, ENABLE);
#if	UART_COMPLTET_IDLE
	volatile uint32_t temp;
	temp																				= USART_Drv->SR;
	temp																				= USART_Drv->DR;
	USART_ITConfig(USART_Drv, USART_IT_IDLE, ENABLE);
#endif

#if !USE_DMA
	USART_ITConfig(USART_Drv, USART_IT_TXE, DISABLE);
#endif
	NVIC_InitStruct.NVIC_IRQChannel							= USART_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority	= 0;
	NVIC_InitStruct.NVIC_IRQChannelSubPriority	= 0;
	NVIC_InitStruct.NVIC_IRQChannelCmd					= ENABLE;
	NVIC_Init(&NVIC_InitStruct);
#if USE_DMA	
	USART_InitDMA();
#endif	
	USART_Cmd(USART_Drv, ENABLE);
	
	timid_USART = xTimerCreate("UsrT1", pdMS_TO_TICKS(1), pdFALSE, NULL, USART_Timer_CallBack);
	xTaskCreate(USART_Thread1, "USART1", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 3, &tid_USART);
	if((!timid_USART) || (!tid_USART))
		while(1);
}

#if USE_DMA
static void	USART_InitDMA(void){
	DMA_InitTypeDef												DMA_InitStruct;
	NVIC_InitTypeDef											NVIC_InitStruct;
	
	DMA_Enable_Clock();
	
	NVIC_InitStruct.NVIC_IRQChannel				= DMA_USART_TxChannel_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority	= 0;
	NVIC_InitStruct.NVIC_IRQChannelSubPriority				= 0;
	NVIC_InitStruct.NVIC_IRQChannelCmd		= ENABLE;
	NVIC_Init(&NVIC_InitStruct);
	
	DMA_InitStruct.DMA_BufferSize					= 0;
	DMA_InitStruct.DMA_DIR								= DMA_DIR_PeripheralDST;
	DMA_InitStruct.DMA_M2M								= DMA_M2M_Disable;
	DMA_InitStruct.DMA_MemoryBaseAddr			= 0;
	DMA_InitStruct.DMA_MemoryDataSize			= DMA_MemoryDataSize_Byte;
	DMA_InitStruct.DMA_MemoryInc					= DMA_MemoryInc_Enable;
	DMA_InitStruct.DMA_Mode								= DMA_Mode_Normal;
	DMA_InitStruct.DMA_PeripheralBaseAddr	= (uint32_t)&USART_Drv->DR;
	DMA_InitStruct.DMA_PeripheralDataSize	= DMA_PeripheralDataSize_Byte;
	DMA_InitStruct.DMA_PeripheralInc			= DMA_PeripheralInc_Disable;
	DMA_InitStruct.DMA_Priority						= DMA_Priority_Medium;
	DMA_Init(DMA_USART_TxChannel, &DMA_InitStruct);
	
	DMA_ITConfig(DMA_USART_TxChannel, DMA_IT_TC, ENABLE);
	DMA_ITConfig(DMA_USART_TxChannel, DMA_IT_TE, ENABLE);
	
	DMA_Cmd(DMA_USART_TxChannel, DISABLE);
	
	USART_DMACmd(USART_Drv, USART_DMAReq_Tx, ENABLE);
//	USART_DMACmd(USART_Drv, USART_DMAReq_Rx, ENABLE);

}

void	DMA_USART_TX_IRQHandler(void){
	if(DMA_GetITStatus(DMA_USART_TX_IT_TC) != RESET){
		pDMA_TxCh_CCR->EN										= 0;
		isDMA_Busy													= false;
		DMA_ClearITPendingBit(DMA_USART_TX_IT_TC);
	}
	if(DMA_GetITStatus(DMA_USART_TX_IT_TE) != RESET){
		DMA_ClearITPendingBit(DMA_USART_TX_IT_TE);
	}
}
#endif


void	USART_ISR(void){
	volatile	uint32_t	temp;
	if(USART_GetITStatus(USART_Drv, USART_IT_ORE) != RESET){
		temp							= USART_Drv->SR;
		temp							= USART_Drv->DR;
		USART_ClearFlag(USART_Drv, USART_IT_ORE);
		return;
	}
	if(USART_GetITStatus(USART_Drv, USART_IT_RXNE) != RESET){
		pRxBuf[RxCount++]		= USART_Drv->DR;
//		if(/*(USART_Drv->DR == '\r') ||*/ (USART_Drv->DR == '\n') || (RxCount >= RxBufLength)){
//			Event			= 2;
//			xTaskNotifyFromISR(tid_USART, 0x55aa, eSetBits, NULL);
//		}
//		else{
//			Event			= 1;
//			xTaskNotifyFromISR(tid_USART, 0x55aa, eSetBits, NULL);
//		}
		USART_ClearFlag(USART_Drv, USART_IT_RXNE);
		return;
	}
	if(USART_GetITStatus(USART_Drv, USART_IT_TC) != RESET){
		USART_ClearITPendingBit(USART_Drv, USART_IT_TC);
		if(!pTxBuf){
			if(TxCount < TxLen){
				USART_Drv->DR	= pTxBuf[TxCount];
				TxCount++;
			}
			else{
				USART_ITConfig(USART_Drv, USART_IT_TC, DISABLE);
				isDMA_Busy		= false;
				pTxBuf				= NULL;
				TxLen					= 0;
				TxCount				= 0;
			}
		}
		else{
			USART_ITConfig(USART_Drv, USART_IT_TC, DISABLE);
			isDMA_Busy			= false;
			TxLen						= 0;
			TxCount					= 0;
		}
	}
#if		UART_COMPLTET_IDLE
	if(USART_GetITStatus(USART_Drv, USART_IT_IDLE) != RESET){
		temp							= USART_Drv->SR;
		temp							= USART_Drv->DR;
		USART_ClearFlag(USART_Drv, USART_IT_IDLE);
		Event							= 2;
		xTaskNotifyFromISR(tid_USART, 0x55aa, eSetBits, NULL);
	}
#endif
}