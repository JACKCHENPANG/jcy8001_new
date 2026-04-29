/**
 * JCY8001 极简USART2发送测试
 * 只做一件事：不断发送 0x55 ('U')
 * 排除一切中断/Modbus/LED干扰
 */
#include <stdint.h>

#define PERIPH_BASE       0x40000000UL
#define APB1PERIPH_BASE   PERIPH_BASE
#define APB2PERIPH_BASE   (PERIPH_BASE + 0x10000UL)
#define AHBPERIPH_BASE    (PERIPH_BASE + 0x20000UL)

#define GPIOA_BASE        (APB2PERIPH_BASE + 0x0800UL)
#define GPIOA             ((volatile uint32_t *)GPIOA_BASE)
/* GPIOA registers: CRL=0, CRH=1, IDR=2, ODR=3, BSRR=4, BRR=5 */
#define GPIOA_CRL         GPIOA[0]
#define GPIOA_ODR         GPIOA[3]
#define GPIOA_BSRR        GPIOA[4]
#define GPIOA_BRR         GPIOA[5]

#define RCC_BASE          (AHBPERIPH_BASE + 0x1000UL)
#define RCC               ((volatile uint32_t *)RCC_BASE)
#define RCC_CR            RCC[0]
#define RCC_CFGR          RCC[1]
#define RCC_APB2ENR       RCC[6]
#define RCC_APB1ENR       RCC[7]

#define FLASH_ACR         (*((volatile uint32_t *)0x40022000UL))

#define AFIO_MAPR         (*((volatile uint32_t *)0x40010004UL))

#define USART2_BASE       (APB1PERIPH_BASE + 0x4400UL)
#define USART2            ((volatile uint32_t *)USART2_BASE)
#define USART2_SR         USART2[0]
#define USART2_DR         USART2[1]
#define USART2_BRR        USART2[2]
#define USART2_CR1        USART2[3]

#define SCB_VTOR          (*((volatile uint32_t *)0xE000ED08UL))

void SystemInit(void)
{
    SCB_VTOR = 0x08000000UL;
    
    /* 禁用所有中断，清除挂起 */
    __asm volatile("cpsid i" ::: "memory");
    for (int i = 0; i < 8; i++) {
        ((volatile uint32_t *)0xE000E180UL)[i] = 0xFFFFFFFFUL; /* NVIC_ICPR */
    }
    
    /* Flash: 预取 + 1 wait state */
    FLASH_ACR = 0x12; /* PRFTBE + LATENCY_1 */
    
    /* HSI/2 * 16 = 64MHz */
    RCC_CR |= (1UL << 0);  /* HSION */
    RCC_CFGR &= ~((1UL << 16) | (1UL << 17) | (0xFUL << 18)); /* clear PLL config */
    RCC_CFGR |= (0xFUL << 18);  /* PLLMULL = x16 */
    RCC_CFGR |= (4UL << 8);     /* PPRE1 = /2 */
    
    RCC_CR |= (1UL << 24);  /* PLLON */
    while (!(RCC_CR & (1UL << 25))); /* PLLRDY */
    
    RCC_CFGR &= ~(3UL << 0);
    RCC_CFGR |= (2UL << 0);  /* SW = PLL */
    while ((RCC_CFGR & (3UL << 2)) != (2UL << 2)); /* SWS = PLL */
    
    __asm volatile("cpsie i" ::: "memory");
}

int main(void)
{
    /* 1. 使能时钟 */
    RCC_APB2ENR |= (1UL << 0) | (1UL << 2);  /* AFIOEN + IOPAEN */
    RCC_APB1ENR |= (1UL << 17);               /* USART2EN */
    
    /* 2. AFIO_MAPR: 禁用JTAG保留SWD (SWJ_CFG=010)
     * 释放PA3(JTDO)/PA4(JNTRST)/PA15(JTDI) */
    AFIO_MAPR = 0x02000000UL;
    
    /* 3. GPIO配置 */
    uint32_t crl = GPIOA_CRL;
    /* PA1: Output PP 10MHz = 0x1 (原版固件配置) */
    crl &= ~(0xFUL << 4);
    crl |= (0x1UL << 4);
    /* PA2: AF_PP 50MHz = 0xB (USART2 TX) */
    crl &= ~(0xFUL << 8);
    crl |= (0xBUL << 8);
    /* PA3: Input PU/PD = 0x8 (USART2 RX) */
    crl &= ~(0xFUL << 12);
    crl |= (0x8UL << 12);
    GPIOA_CRL = crl;
    
    /* PA1拉高 (原版固件行为) */
    GPIOA_BSRR = (1UL << 1);
    
    /* 4. USART2: 115200, 8N1, 只发不收 */
    USART2_BRR = 278;  /* 32MHz/115200 ≈ 278 */
    USART2_CR1 = (1UL << 13) | (1UL << 3);  /* UE + TE */
    
    /* 5. 等一下让USART稳定 */
    for (volatile int i = 0; i < 10000; i++);
    
    /* 6. 发送 0x55 ('U') = 01010101, 持续不断 */
    while (1) {
        while (!(USART2_SR & (1UL << 7))); /* 等TXE */
        USART2_DR = 0x55;
    }
}

/* 空的USART2中断处理（startup.s不再用.weak） */
void USART2_IRQHandler(void) {}
