



#ifndef _MY_DEFINE_H_
#define _MY_DEFINE_H_







typedef enum{
	Status_OK	= 0,
	Status_Busy,
	Status_Error,
	Status_Waiting,
	Status_Processing,
	Status_TimeOut,
	Status_E_Stop,
	Status_Limit_Zero,
	Status_Limit_End,
	Status_Limit_Up,
	Status_Limit_Dn,
	Status_OCP,
	Status_OVP,
	Status_Inv,
	Status_DrvErr,
	Status_Param,
	Status_NotCalMode,
	Status_NoSupport,
	Status_RampMode,
	Status_CalMode,
	Status_Mode,
	Status_TempOver,
	Status_OCL,
	Status_Short,

	Status_RemoteMode,
	Status_LocalMode,
	
	Status_CRC,
	Status_ID,
	Status_ID_Zero,
	
	Status_Overrun,
}Status_t;



typedef void (*pFunction)(void);


#ifndef NULL
#define	NULL		0
#endif









#endif

