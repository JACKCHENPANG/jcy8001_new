/**
 * system_stm32f10x.c - 系统初始化
 * 配置 STM32F103RC 为 72MHz
 */

#include "stm32f10x.h"

// 系统时钟频率
uint32_t SystemCoreClock = 72000000;

// 调试标志（放在 .noinit 段，避免被启动代码清零）
__attribute__((section(".noinit"))) volatile uint32_t rcc_config_done;

// AHB 预分频表
const uint8_t AHBPrescTable[16U] = {0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 1U, 2U, 3U, 4U, 6U, 7U, 8U, 9U};
const uint8_t APBPrescTable[8U] =  {0U, 0U, 0U, 0U, 1U, 2U, 3U, 4U};

// 前向声明
void RCC_Configuration(void);

/**
 * 系统初始化 - 配置 72MHz
 */
void SystemInit(void)
{
    // 设置向量表偏移
    SCB->VTOR = 0x08000000U;
    
    // 复位 RCC 时钟配置
    RCC->CR |= RCC_CR_HSION;           // 使能 HSI
    RCC->CFGR &= ~(RCC_CFGR_SW | RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2);
    
    // 禁用所有中断并清除挂起状态
    __disable_irq();
    for (int i = 0; i < 8; i++) {
        NVIC->ICER[i] = 0xFFFFFFFFU;
        NVIC->ICPR[i] = 0xFFFFFFFFU;
    }
    __enable_irq();
    
    // 配置系统时钟为 72MHz
    RCC_Configuration();
}

/**
 * 配置系统时钟为 72MHz
 * 使用 HSI 内部时钟（因为 HSE 可能未焊接）
 */
void RCC_Configuration(void)
{
    volatile uint32_t timeout = 0xFFFF;
    
    rcc_config_done = 0xAA;  // 标记进入函数
    
    // 使用 HSI/2 * 16 = 64MHz（STM32F103 HSI 最大倍频 16）
    // 注意：STM32F103 使用 HSI 时最大只能到 64MHz
    
    // 配置 Flash 延迟（64MHz 需要 1 个等待周期）
    FLASH->ACR = FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY_1;
    
    // 配置 PLL: HSI/2 * 16 = 64MHz
    RCC->CFGR &= ~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLXTPRE | RCC_CFGR_PLLMULL);
    RCC->CFGR |= RCC_CFGR_PLLMULL16;  // HSI/2 * 16 = 64MHz
    
    // 配置总线分频
    RCC->CFGR &= ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2);
    RCC->CFGR |= RCC_CFGR_PPRE1_DIV2;  // APB1 = 32MHz
    
    // 使能 PLL
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY) && --timeout);
    
    if (RCC->CR & RCC_CR_PLLRDY) {
        // 切换到 PLL
        RCC->CFGR &= ~RCC_CFGR_SW;
        RCC->CFGR |= RCC_CFGR_SW_PLL;
        while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);
        
        SystemCoreClock = 64000000;  // 64MHz
        rcc_config_done = 1;  // 标记完成
    } else {
        // PLL 失败，使用 HSI 8MHz
        SystemCoreClock = 8000000;
        rcc_config_done = 2;  // 标记失败
    }
}

/**
 * 更新系统时钟
 */
void SystemCoreClockUpdate(void)
{
    uint32_t tmp = RCC->CFGR & RCC_CFGR_SWS;
    
    switch (tmp) {
        case 0x00:  // HSI
            SystemCoreClock = 8000000;
            break;
        case 0x04:  // HSE
            SystemCoreClock = 8000000;
            break;
        case 0x08:  // PLL
            SystemCoreClock = 72000000;
            break;
    }
}
