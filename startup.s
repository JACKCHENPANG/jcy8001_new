/**
 * startup.s - STM32F103RC GNU汇编启动文件
 * 最小化版本，只保留必要的异常向量
 */
    .syntax unified
    .cpu cortex-m3
    .fpu softvfp
    .thumb

/* 向量表 */
    .section .isr_vector, "a", %progbits
    .global _estack
    .type g_pfnVectors, %object
g_pfnVectors:
    .word _estack              /* 初始栈指针 */
    .word Reset_Handler        /* Reset */
    .word NMI_Handler          /* NMI */
    .word HardFault_Handler    /* HardFault */
    .word MemManage_Handler    /* MemManage */
    .word BusFault_Handler     /* BusFault */
    .word UsageFault_Handler   /* UsageFault */
    .word 0                    /* Reserved */
    .word 0                    /* Reserved */
    .word 0                    /* Reserved */
    .word 0                    /* Reserved */
    .word SVC_Handler          /* SVCall */
    .word DebugMon_Handler     /* Debug Monitor */
    .word 0                    /* Reserved */
    .word PendSV_Handler       /* PendSV */
    .word SysTick_Handler      /* SysTick */

    /* 外部中断 - STM32F103RC HD 有60个 */
    .word WWDG_IRQHandler      /* 0: Window Watchdog */
    .word PVD_IRQHandler       /* 1: PVD */
    .word TAMPER_IRQHandler    /* 2: Tamper */
    .word RTC_IRQHandler       /* 3: RTC */
    .word FLASH_IRQHandler     /* 4: Flash */
    .word RCC_IRQHandler       /* 5: RCC */
    .word EXTI0_IRQHandler     /* 6: EXTI0 */
    .word EXTI1_IRQHandler     /* 7: EXTI1 */
    .word EXTI2_IRQHandler     /* 8: EXTI2 */
    .word EXTI3_IRQHandler     /* 9: EXTI3 */
    .word EXTI4_IRQHandler     /* 10: EXTI4 */
    .word DMA1_Channel1_IRQHandler  /* 11 */
    .word DMA1_Channel2_IRQHandler  /* 12 */
    .word DMA1_Channel3_IRQHandler  /* 13 */
    .word DMA1_Channel4_IRQHandler  /* 14 */
    .word DMA1_Channel5_IRQHandler  /* 15 */
    .word DMA1_Channel6_IRQHandler  /* 16 */
    .word DMA1_Channel7_IRQHandler  /* 17 */
    .word ADC1_2_IRQHandler    /* 18: ADC1/2 */
    .word USB_HP_CAN1_TX_IRQHandler  /* 19 */
    .word USB_LP_CAN1_RX0_IRQHandler /* 20 */
    .word CAN1_RX1_IRQHandler  /* 21 */
    .word CAN1_SCE_IRQHandler  /* 22 */
    .word EXTI9_5_IRQHandler   /* 23: EXTI9-5 */
    .word TIM1_BRK_IRQHandler  /* 24 */
    .word TIM1_UP_IRQHandler   /* 25 */
    .word TIM1_TRG_COM_IRQHandler /* 26 */
    .word TIM1_CC_IRQHandler   /* 27 */
    .word TIM2_IRQHandler      /* 28 */
    .word TIM3_IRQHandler      /* 29 */
    .word TIM4_IRQHandler      /* 30 */
    .word I2C1_EV_IRQHandler   /* 31 */
    .word I2C1_ER_IRQHandler   /* 32 */
    .word I2C2_EV_IRQHandler   /* 33 */
    .word I2C2_ER_IRQHandler   /* 34 */
    .word SPI1_IRQHandler      /* 35 */
    .word SPI2_IRQHandler      /* 36 */
    .word USART1_IRQHandler    /* 37: USART1 ★ */
    .word USART2_IRQHandler    /* 38: USART2 */
    .word USART3_IRQHandler    /* 39: USART3 */
    .word EXTI15_10_IRQHandler /* 40: EXTI15-10 */
    .word RTCAlarm_IRQHandler  /* 41 */
    .word USBWakeUp_IRQHandler /* 42 */
    .word TIM8_BRK_IRQHandler  /* 43 */
    .word TIM8_UP_IRQHandler   /* 44 */
    .word TIM8_TRG_COM_IRQHandler /* 45 */
    .word TIM8_CC_IRQHandler   /* 46 */
    .word ADC3_IRQHandler      /* 47 */
    .word FSMC_IRQHandler      /* 48 */
    .word SDIO_IRQHandler      /* 49 */
    .word TIM5_IRQHandler      /* 50 */
    .word SPI3_IRQHandler      /* 51 */
    .word UART4_IRQHandler     /* 52 */
    .word UART5_IRQHandler     /* 53 */
    .word TIM6_IRQHandler      /* 54 */
    .word TIM7_IRQHandler      /* 55 */
    .word DMA2_Channel1_IRQHandler /* 56 */
    .word DMA2_Channel2_IRQHandler /* 57 */
    .word DMA2_Channel3_IRQHandler /* 58 */
    .word DMA2_Channel4_5_IRQHandler /* 59 */

    .size g_pfnVectors, . - g_pfnVectors

/* Reset Handler */
    .section .text
    .type Reset_Handler, %function
    .global Reset_Handler
    .weak Reset_Handler
Reset_Handler:
    /* 调用 SystemInit（时钟配置，在数据段初始化之前） */
    bl SystemInit
    /* 复制 .data 段从 Flash 到 RAM */
    ldr r0, =_sdata
    ldr r1, =_edata
    ldr r2, =_sidata
copy_data:
    cmp r0, r1
    bcc copy_data_loop
    b zero_bss
copy_data_loop:
    ldr r3, [r2]
    str r3, [r0]
    adds r0, r0, #4
    adds r2, r2, #4
    b copy_data

    /* 清零 .bss 段 */
zero_bss:
    ldr r0, =_sbss
    ldr r1, =_ebss
    movs r2, #0
zero_bss_loop:
    cmp r0, r1
    bcc zero_bss_fill
    b call_main
zero_bss_fill:
    str r2, [r0]
    adds r0, r0, #4
    b zero_bss_loop

call_main:
    /* 调用 main */
    bl main
    /* 如果 main 返回，死循环 */
    b .

/* 默认中断处理 - 死循环 */
    .type Default_Handler, %function
    .global Default_Handler
    .weak Default_Handler
Default_Handler:
    b .

/* 弱符号绑定 - 所有中断默认指向 Default_Handler */
    .weak NMI_Handler
    .thumb_set NMI_Handler, Default_Handler
    .weak HardFault_Handler
    .thumb_set HardFault_Handler, Default_Handler
    .weak MemManage_Handler
    .thumb_set MemManage_Handler, Default_Handler
    .weak BusFault_Handler
    .thumb_set BusFault_Handler, Default_Handler
    .weak UsageFault_Handler
    .thumb_set UsageFault_Handler, Default_Handler
    .weak SVC_Handler
    .thumb_set SVC_Handler, Default_Handler
    .weak DebugMon_Handler
    .thumb_set DebugMon_Handler, Default_Handler
    .weak PendSV_Handler
    .thumb_set PendSV_Handler, Default_Handler
    .weak SysTick_Handler
    .thumb_set SysTick_Handler, Default_Handler

    .weak WWDG_IRQHandler
    .thumb_set WWDG_IRQHandler, Default_Handler
    .weak PVD_IRQHandler
    .thumb_set PVD_IRQHandler, Default_Handler
    .weak TAMPER_IRQHandler
    .thumb_set TAMPER_IRQHandler, Default_Handler
    .weak RTC_IRQHandler
    .thumb_set RTC_IRQHandler, Default_Handler
    .weak FLASH_IRQHandler
    .thumb_set FLASH_IRQHandler, Default_Handler
    .weak RCC_IRQHandler
    .thumb_set RCC_IRQHandler, Default_Handler
    .weak EXTI0_IRQHandler
    .thumb_set EXTI0_IRQHandler, Default_Handler
    .weak EXTI1_IRQHandler
    .thumb_set EXTI1_IRQHandler, Default_Handler
    .weak EXTI2_IRQHandler
    .thumb_set EXTI2_IRQHandler, Default_Handler
    .weak EXTI3_IRQHandler
    .thumb_set EXTI3_IRQHandler, Default_Handler
    .weak EXTI4_IRQHandler
    .thumb_set EXTI4_IRQHandler, Default_Handler
    .weak DMA1_Channel1_IRQHandler
    .thumb_set DMA1_Channel1_IRQHandler, Default_Handler
    .weak DMA1_Channel2_IRQHandler
    .thumb_set DMA1_Channel2_IRQHandler, Default_Handler
    .weak DMA1_Channel3_IRQHandler
    .thumb_set DMA1_Channel3_IRQHandler, Default_Handler
    .weak DMA1_Channel4_IRQHandler
    .thumb_set DMA1_Channel4_IRQHandler, Default_Handler
    .weak DMA1_Channel5_IRQHandler
    .thumb_set DMA1_Channel5_IRQHandler, Default_Handler
    .weak DMA1_Channel6_IRQHandler
    .thumb_set DMA1_Channel6_IRQHandler, Default_Handler
    .weak DMA1_Channel7_IRQHandler
    .thumb_set DMA1_Channel7_IRQHandler, Default_Handler
    .weak ADC1_2_IRQHandler
    .thumb_set ADC1_2_IRQHandler, Default_Handler
    .weak USB_HP_CAN1_TX_IRQHandler
    .thumb_set USB_HP_CAN1_TX_IRQHandler, Default_Handler
    .weak USB_LP_CAN1_RX0_IRQHandler
    .thumb_set USB_LP_CAN1_RX0_IRQHandler, Default_Handler
    .weak CAN1_RX1_IRQHandler
    .thumb_set CAN1_RX1_IRQHandler, Default_Handler
    .weak CAN1_SCE_IRQHandler
    .thumb_set CAN1_SCE_IRQHandler, Default_Handler
    .weak EXTI9_5_IRQHandler
    .thumb_set EXTI9_5_IRQHandler, Default_Handler
    .weak TIM1_BRK_IRQHandler
    .thumb_set TIM1_BRK_IRQHandler, Default_Handler
    .weak TIM1_UP_IRQHandler
    .thumb_set TIM1_UP_IRQHandler, Default_Handler
    .weak TIM1_TRG_COM_IRQHandler
    .thumb_set TIM1_TRG_COM_IRQHandler, Default_Handler
    .weak TIM1_CC_IRQHandler
    .thumb_set TIM1_CC_IRQHandler, Default_Handler
    .weak TIM2_IRQHandler
    .thumb_set TIM2_IRQHandler, Default_Handler
    .weak TIM3_IRQHandler
    .thumb_set TIM3_IRQHandler, Default_Handler
    .weak TIM4_IRQHandler
    .thumb_set TIM4_IRQHandler, Default_Handler
    .weak I2C1_EV_IRQHandler
    .thumb_set I2C1_EV_IRQHandler, Default_Handler
    .weak I2C1_ER_IRQHandler
    .thumb_set I2C1_ER_IRQHandler, Default_Handler
    .weak I2C2_EV_IRQHandler
    .thumb_set I2C2_EV_IRQHandler, Default_Handler
    .weak I2C2_ER_IRQHandler
    .thumb_set I2C2_ER_IRQHandler, Default_Handler
    .weak SPI1_IRQHandler
    .thumb_set SPI1_IRQHandler, Default_Handler
    .weak SPI2_IRQHandler
    .thumb_set SPI2_IRQHandler, Default_Handler
    .weak USART1_IRQHandler
    .thumb_set USART1_IRQHandler, Default_Handler
    /* USART2 不用弱符号 - 在 main.c 中定义 */
    .weak USART3_IRQHandler
    .thumb_set USART3_IRQHandler, Default_Handler
    .weak EXTI15_10_IRQHandler
    .thumb_set EXTI15_10_IRQHandler, Default_Handler
    .weak RTCAlarm_IRQHandler
    .thumb_set RTCAlarm_IRQHandler, Default_Handler
    .weak USBWakeUp_IRQHandler
    .thumb_set USBWakeUp_IRQHandler, Default_Handler
    .weak TIM8_BRK_IRQHandler
    .thumb_set TIM8_BRK_IRQHandler, Default_Handler
    .weak TIM8_UP_IRQHandler
    .thumb_set TIM8_UP_IRQHandler, Default_Handler
    .weak TIM8_TRG_COM_IRQHandler
    .thumb_set TIM8_TRG_COM_IRQHandler, Default_Handler
    .weak TIM8_CC_IRQHandler
    .thumb_set TIM8_CC_IRQHandler, Default_Handler
    .weak ADC3_IRQHandler
    .thumb_set ADC3_IRQHandler, Default_Handler
    .weak FSMC_IRQHandler
    .thumb_set FSMC_IRQHandler, Default_Handler
    .weak SDIO_IRQHandler
    .thumb_set SDIO_IRQHandler, Default_Handler
    .weak TIM5_IRQHandler
    .thumb_set TIM5_IRQHandler, Default_Handler
    .weak SPI3_IRQHandler
    .thumb_set SPI3_IRQHandler, Default_Handler
    .weak UART4_IRQHandler
    .thumb_set UART4_IRQHandler, Default_Handler
    .weak UART5_IRQHandler
    .thumb_set UART5_IRQHandler, Default_Handler
    .weak TIM6_IRQHandler
    .thumb_set TIM6_IRQHandler, Default_Handler
    .weak TIM7_IRQHandler
    .thumb_set TIM7_IRQHandler, Default_Handler
    .weak DMA2_Channel1_IRQHandler
    .thumb_set DMA2_Channel1_IRQHandler, Default_Handler
    .weak DMA2_Channel2_IRQHandler
    .thumb_set DMA2_Channel2_IRQHandler, Default_Handler
    .weak DMA2_Channel3_IRQHandler
    .thumb_set DMA2_Channel3_IRQHandler, Default_Handler
    .weak DMA2_Channel4_5_IRQHandler
    .thumb_set DMA2_Channel4_5_IRQHandler, Default_Handler

    .end
