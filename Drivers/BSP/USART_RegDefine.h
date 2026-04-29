


#ifndef _USART_REG_DEFINE_H_
#define _USART_REG_DEFINE_H_


#include "stdint.h"

typedef struct{
	uint32_t		PE		: 1;
	uint32_t		FE		: 1;
	uint32_t		NF		: 1;
	uint32_t		ORE		: 1;
	uint32_t		IDLE	: 1;
	uint32_t		RXNE	: 1;
	uint32_t		TC		: 1;
	uint32_t		TXE		: 1;
	uint32_t		LBD		: 1;
	uint32_t		CTS		: 1;
	uint32_t		RES		: 22;
}USART_SR_Reg_t;
	
typedef struct{
	uint32_t		SBK				: 1;		//发送断路字符
	uint32_t		RWU				: 1;		//接收器唤醒
	uint32_t		RE				: 1;		//接收器使能
	uint32_t		TE				: 1;		//发送器使能
	uint32_t		IDLEIE		: 1;
	uint32_t		RXNEIE		: 1;
	uint32_t		TCIE			: 1;		
	uint32_t		TXEIE			: 1;		//TXE中断使能
	uint32_t		PEIE			: 1;		//PE中断使能
	uint32_t		PS				: 1;		//奇偶校验选择
	uint32_t		PCE				: 1;		//奇偶校验使能
	uint32_t		WAKE			: 1;		//唤醒方法
	uint32_t		M					: 1;		//字长0 - 8bits , 1 - 9bits
	uint32_t		UE				: 1;		//USART EN
	uint32_t		RES1			: 1;
	uint32_t		OVER8			: 1;		//8倍过采样
	uint32_t		RES2			: 16;
}USART_CR1_Reg_t;

typedef struct{
	uint32_t		ADD				: 4;		//USART节点地址
	uint32_t		RES1			: 1;
	uint32_t		LBDL			: 1;		//LIN断路检测长度
	uint32_t		LBDIE			: 1;		//LIN断路检测中断使能
	uint32_t		RES2			: 1;
	uint32_t		LBCL			: 1;		//同步模式的最后一个时钟脉冲
	uint32_t		CPHA			: 1;		//同步模式的时钟相位
	uint32_t		CPOL			: 1;		//同步模式的时钟极性
	uint32_t		CLKEN			: 1;		//时钟使能
	uint32_t		STOP			: 2;		//停止位
	uint32_t		LENEN			: 1;		//LIN模式使能
	uint32_t		RES3			: 17;
}USART_CR2_Reg_t;



typedef struct{
	uint32_t		EIE			: 1;		//错误中断使能
	uint32_t		IREN		: 1;		//IrDA模式使能
	uint32_t		IRLP		: 1;		//IrDA低功耗
	uint32_t		HDSEL		: 1;		//半双工模式使能
	uint32_t		NACK		: 1;		//智能卡NACK使能
	uint32_t		SCEN		: 1;		//智能卡模式使能
	uint32_t		DMAR		: 1;		//DMA使能接收器
	uint32_t		DMAT		: 1;		//DMA使能发送器
	uint32_t		RTSE		: 1;		//RTS使能
	uint32_t		CTSE		: 1;		//CTS使能
	uint32_t		CTSIE		: 1;		//CTS中断使能
	uint32_t		ONEBIT	: 1;		//1个采样位方式
	uint32_t		RESE		: 20;
}USART_CR3_Reg_t;












#endif
