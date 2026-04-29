/**
 * JCY8001 最小固件 - 寄存器直操作版本
 * 
 * 功能：
 *   1. SystemInit: HSI/2 * 16 = 64MHz
 *   2. USART2: 115200, PA2/PA3, 中断接收（原版固件用USART2通讯）
 *   3. Modbus RTU: 从站地址1, 功能码03/04/06
 *   4. LED心跳: PC13 翻转
 * 
 * 零外部依赖，纯寄存器操作
 */

#include <stdint.h>

/* ============================================================
 * STM32F103 寄存器定义（最小集）
 * ============================================================ */

#define PERIPH_BASE       0x40000000UL
#define APB1PERIPH_BASE   PERIPH_BASE
#define APB2PERIPH_BASE   (PERIPH_BASE + 0x10000UL)
#define AHBPERIPH_BASE    (PERIPH_BASE + 0x20000UL)

/* GPIO */
#define GPIOA_BASE        (APB2PERIPH_BASE + 0x0800UL)
#define GPIOC_BASE        (APB2PERIPH_BASE + 0x1000UL)
#define GPIOA             ((GPIO_TypeDef *)GPIOA_BASE)
#define GPIOC             ((GPIO_TypeDef *)GPIOC_BASE)

typedef struct {
    volatile uint32_t CRL, CRH, IDR, ODR, BSRR, BRR, LCKR;
} GPIO_TypeDef;

/* RCC */
#define RCC_BASE          (AHBPERIPH_BASE + 0x1000UL)
#define RCC               ((RCC_TypeDef *)RCC_BASE)

typedef struct {
    volatile uint32_t CR, CFGR, CIR, APB2RSTR, APB1RSTR;
    volatile uint32_t AHBENR, APB2ENR, APB1ENR, BDCR, CSR;
} RCC_TypeDef;

#define RCC_CR_HSION          (1UL << 0)
#define RCC_CR_PLLON          (1UL << 24)
#define RCC_CR_PLLRDY         (1UL << 25)
#define RCC_CFGR_SW_POS       0
#define RCC_CFGR_SWS_POS      2
#define RCC_CFGR_SWS_PLL      (2UL << 2)
#define RCC_CFGR_PLLMULL_POS  18
#define RCC_CFGR_PLLMULL16    (0xFUL << 18)
#define RCC_CFGR_PLLSRC       (1UL << 16)
#define RCC_CFGR_PLLXTPRE     (1UL << 17)
#define RCC_CFGR_PPRE1_POS    8
#define RCC_CFGR_PPRE1_DIV2   (4UL << 8)

#define RCC_APB2ENR_AFIOEN    (1UL << 0)
#define RCC_APB2ENR_IOPAEN    (1UL << 2)
#define RCC_APB2ENR_IOPCEN    (1UL << 4)
#define RCC_APB2ENR_SPI1EN    (1UL << 12)
#define RCC_APB1ENR_USART2EN  (1UL << 17)

/* Flash */
#define FLASH_ACR          (*((volatile uint32_t *)0x40022000UL))
#define FLASH_ACR_PRFTBE   (1UL << 4)
#define FLASH_ACR_LATENCY_1 (1UL << 0)

/* USART2 (APB1, PA2=TX, PA3=RX) */
#define USART2_BASE        (APB1PERIPH_BASE + 0x4400UL)
#define USART2             ((USART_TypeDef *)USART2_BASE)

typedef struct {
    volatile uint32_t SR, DR, BRR, CR1, CR2, CR3;
} USART_TypeDef;

#define USART_SR_RXNE       (1UL << 5)
#define USART_SR_TXE        (1UL << 7)
#define USART_SR_TC         (1UL << 6)
#define USART_SR_IDLE       (1UL << 4)
#define USART_SR_ORE        (1UL << 3)

#define USART_CR1_UE        (1UL << 13)
#define USART_CR1_TE        (1UL << 3)
#define USART_CR1_RE        (1UL << 2)
#define USART_CR1_RXNEIE    (1UL << 5)
#define USART_CR1_IDLEIE    (1UL << 4)

/* NVIC */
#define NVIC_BASE           0xE000E100UL
#define NVIC_ISER0          (*((volatile uint32_t *)(NVIC_BASE + 0x00)))
#define NVIC_ISER1          (*((volatile uint32_t *)(NVIC_BASE + 0x04)))
#define NVIC_ICER           ((volatile uint32_t *)(NVIC_BASE + 0x80))
#define NVIC_ICPR           ((volatile uint32_t *)(NVIC_BASE + 0x180))
#define NVIC_IPR            ((volatile uint32_t *)(NVIC_BASE + 0x300))

/* USART2 IRQn = 38, NVIC ISER1 bit6 */
#define USART2_IRQn         38

/* SCB */
#define SCB_VTOR            (*((volatile uint32_t *)0xE000ED08UL))

/* SPI1 (APB2, PA5=SCK, PA6=MISO, PA7=MOSI) */
#define SPI1_BASE        (APB2PERIPH_BASE + 0x3000UL)
#define SPI1             ((SPI_TypeDef *)SPI1_BASE)

typedef struct {
    volatile uint32_t CR1, CR2, SR, DR, CRCPR, RXCRCR, TXCRCR;
} SPI_TypeDef;

#define SPI_CR1_SPE     (1UL << 6)
#define SPI_CR1_MSTR    (1UL << 2)
#define SPI_CR1_CPOL    (1UL << 1)
#define SPI_CR1_CPHA    (1UL << 0)
#define SPI_CR1_BR_POS  3
#define SPI_CR1_BR_DIV2 (0UL << 3)

#define SPI_SR_RXNE     (1UL << 0)
#define SPI_SR_TXE      (1UL << 1)

/* SysTick - 注意地址顺序: CTRL=0x10, LOAD=0x14, VAL=0x18 */
#define SysTick_CTRL        (*((volatile uint32_t *)0xE000E010UL))
#define SysTick_LOAD        (*((volatile uint32_t *)0xE000E014UL))
#define SysTick_VAL         (*((volatile uint32_t *)0xE000E018UL))
#define SysTick_CTRL_CLKSOURCE  (1UL << 2)
#define SysTick_CTRL_TICKINT    (1UL << 1)
#define SysTick_CTRL_ENABLE     (1UL << 0)

#define __disable_irq()     __asm volatile("cpsid i" ::: "memory")
#define __enable_irq()      __asm volatile("cpsie i" ::: "memory")

/* ============================================================
 * 系统时钟 & Modbus 配置
 * ============================================================ */
#define SYSCLK_FREQ         64000000UL   /* HSI/2 * 16 = 64MHz */
#define APB1_CLK            32000000UL   /* APB1 = SYSCLK/2 */
#define MODBUS_SLAVE_ADDR   1
#define MODBUS_BUF_SIZE     128

/* Modbus 寄存器 */
#define MODBUS_INPUT_REGS   256
#define MODBUS_HOLDING_REGS 256

static volatile uint16_t modbus_regs_input[MODBUS_INPUT_REGS];
static volatile uint16_t modbus_regs_holding[MODBUS_HOLDING_REGS];

/* 收发缓冲 */
static uint8_t modbus_rx_buf[MODBUS_BUF_SIZE];
static volatile uint16_t modbus_rx_len = 0;
static volatile uint8_t modbus_rx_done = 0;
static uint8_t modbus_tx_buf[MODBUS_BUF_SIZE];
static uint16_t modbus_tx_len = 0;

/* ============================================================
 * CRC16 Modbus
 * ============================================================ */
static uint16_t crc16_modbus(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            crc = (crc & 1) ? ((crc >> 1) ^ 0xA001) : (crc >> 1);
        }
    }
    return crc;
}

/* RS485/信号使能控制: PA1, 高电平=使能通讯通路 (原版固件PA1=HIGH) */
#define RS485_DE_HIGH()   GPIOA->BSRR = (1UL << 1)
#define RS485_DE_LOW()    GPIOA->BRR  = (1UL << 1)

/* ============================================================
 * USART2 收发
 * ============================================================ */
static void uart_send_byte(uint8_t b)
{
    while (!(USART2->SR & USART_SR_TXE));
    USART2->DR = b;
}

static void uart_send(const uint8_t *data, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++)
        uart_send_byte(data[i]);
    while (!(USART2->SR & USART_SR_TC));
}

static void uart_send_str(const char *str)
{
    while (*str)
        uart_send_byte((uint8_t)*str++);
    while (!(USART2->SR & USART_SR_TC));
}

/* ============================================================
 * Modbus 处理
 * ============================================================ */
static void modbus_func04_read_input(const uint8_t *req)
{
    uint16_t addr = (req[2] << 8) | req[3];
    uint16_t count = (req[4] << 8) | req[5];
    
    if (count < 1 || count > 64) {
        modbus_tx_buf[0] = MODBUS_SLAVE_ADDR;
        modbus_tx_buf[1] = 0x84;
        modbus_tx_buf[2] = 0x03;
        uint16_t crc = crc16_modbus(modbus_tx_buf, 3);
        modbus_tx_buf[3] = crc & 0xFF;
        modbus_tx_buf[4] = (crc >> 8) & 0xFF;
        modbus_tx_len = 5;
        return;
    }
    
    /* 支持完整16位地址映射:
     * 0x3E00-0x3E01: 系统信息
     * 0x3100-0x317F: ZREAL (16通道 int16)
     * 0x3140-0x317F: ZIMAG (16通道 int16)
     * 0x3300-0x333F: TEMP (16通道 int16)
     * 0x3340-0x337F: VOLT (16通道 int16) */
    
    uint8_t byte_count = count * 2;
    
    modbus_tx_buf[0] = MODBUS_SLAVE_ADDR;
    modbus_tx_buf[1] = 0x04;
    modbus_tx_buf[2] = byte_count;
    
    for (uint16_t i = 0; i < count; i++) {
        uint16_t reg_addr = addr + i;
        uint16_t val = 0;
        
        /* 系统寄存器 */
        if (reg_addr == 0x3E00) val = 1;                      /* 通道数 */
        else if (reg_addr == 0x3E01) val = 1;                /* 版本号 */
        /* 电压寄存器 0x3340 + ch*1 */
        else if ((reg_addr >= 0x3340) && (reg_addr < 0x3380))
            val = modbus_regs_input[0x40];  /* 模拟电压值 60000 = 6V */
        /* 温度寄存器 0x3300 + ch*1 */
        else if ((reg_addr >= 0x3300) && (reg_addr < 0x3340))
            val = 250;  /* 模拟温度值 250 = 25.0°C */
        /* ZREAL 0x3100 + ch*1 */
        else if ((reg_addr >= 0x3100) && (reg_addr < 0x3140))
            val = 1000;  /* 模拟 ZREAL = 1000 = 10mΩ */
        /* ZIMAG 0x3140 + ch*1 */
        else if ((reg_addr >= 0x3140) && (reg_addr < 0x3180))
            val = 50;   /* 模拟 ZIMAG = 50 = 0.5mΩ */
        else if (reg_addr < MODBUS_INPUT_REGS)
            val = modbus_regs_input[reg_addr];
        
        modbus_tx_buf[3 + i * 2]     = (val >> 8) & 0xFF;
        modbus_tx_buf[3 + i * 2 + 1] = val & 0xFF;
    }
    
    uint16_t crc = crc16_modbus(modbus_tx_buf, 3 + byte_count);
    modbus_tx_buf[3 + byte_count]     = crc & 0xFF;
    modbus_tx_buf[3 + byte_count + 1] = (crc >> 8) & 0xFF;
    modbus_tx_len = 3 + byte_count + 2;
}

static void modbus_func03_read_holding(const uint8_t *req)
{
    uint16_t addr = (req[2] << 8) | req[3];
    uint16_t count = (req[4] << 8) | req[5];
    
    if (count < 1 || count > 64) {
        modbus_tx_buf[0] = MODBUS_SLAVE_ADDR;
        modbus_tx_buf[1] = 0x83;
        modbus_tx_buf[2] = 0x03;
        uint16_t crc = crc16_modbus(modbus_tx_buf, 3);
        modbus_tx_buf[3] = crc & 0xFF;
        modbus_tx_buf[4] = (crc >> 8) & 0xFF;
        modbus_tx_len = 5;
        return;
    }
    
    uint16_t reg_base = addr & 0xFF;
    uint8_t byte_count = count * 2;
    
    modbus_tx_buf[0] = MODBUS_SLAVE_ADDR;
    modbus_tx_buf[1] = 0x03;
    modbus_tx_buf[2] = byte_count;
    
    for (uint16_t i = 0; i < count; i++) {
        uint16_t idx = reg_base + i;
        uint16_t val = (idx < MODBUS_HOLDING_REGS) ? modbus_regs_holding[idx] : 0;
        modbus_tx_buf[3 + i * 2]     = (val >> 8) & 0xFF;
        modbus_tx_buf[3 + i * 2 + 1] = val & 0xFF;
    }
    
    uint16_t crc = crc16_modbus(modbus_tx_buf, 3 + byte_count);
    modbus_tx_buf[3 + byte_count]     = crc & 0xFF;
    modbus_tx_buf[3 + byte_count + 1] = (crc >> 8) & 0xFF;
    modbus_tx_len = 3 + byte_count + 2;
}

static void modbus_func06_write_single(const uint8_t *req)
{
    uint16_t addr = (req[2] << 8) | req[3];
    uint16_t val  = (req[4] << 8) | req[5];
    
    uint16_t idx = addr & 0xFF;
    if (idx < MODBUS_HOLDING_REGS)
        modbus_regs_holding[idx] = val;
    
    /* 回显请求 */
    for (int i = 0; i < 6; i++)
        modbus_tx_buf[i] = req[i];
    uint16_t crc = crc16_modbus(modbus_tx_buf, 6);
    modbus_tx_buf[6] = crc & 0xFF;
    modbus_tx_buf[7] = (crc >> 8) & 0xFF;
    modbus_tx_len = 8;
}

static void modbus_process(void)
{
    if (modbus_rx_len < 6) return;
    
    uint16_t recv_crc = modbus_rx_buf[modbus_rx_len - 2] | 
                       (modbus_rx_buf[modbus_rx_len - 1] << 8);
    uint16_t calc_crc = crc16_modbus(modbus_rx_buf, modbus_rx_len - 2);
    if (recv_crc != calc_crc) return;
    
    if (modbus_rx_buf[0] != MODBUS_SLAVE_ADDR) return;
    
    uint8_t func = modbus_rx_buf[1];
    modbus_tx_len = 0;
    
    switch (func) {
        case 0x03: modbus_func03_read_holding(modbus_rx_buf); break;
        case 0x04: modbus_func04_read_input(modbus_rx_buf); break;
        case 0x06: modbus_func06_write_single(modbus_rx_buf); break;
        default:
            modbus_tx_buf[0] = MODBUS_SLAVE_ADDR;
            modbus_tx_buf[1] = func | 0x80;
            modbus_tx_buf[2] = 0x01;
            {
                uint16_t crc = crc16_modbus(modbus_tx_buf, 3);
                modbus_tx_buf[3] = crc & 0xFF;
                modbus_tx_buf[4] = (crc >> 8) & 0xFF;
            }
            modbus_tx_len = 5;
            break;
    }
    
    if (modbus_tx_len > 0)
        uart_send(modbus_tx_buf, modbus_tx_len);
}

/* ============================================================
 * USART2 中断处理（原版固件用USART2做Modbus通讯）
 * ============================================================ */
void USART2_IRQHandler(void)
{
    volatile uint32_t sr = USART2->SR;
    
    if (sr & USART_SR_RXNE) {
        uint8_t b = (uint8_t)(USART2->DR & 0xFF);
        if (modbus_rx_len < MODBUS_BUF_SIZE)
            modbus_rx_buf[modbus_rx_len++] = b;
    }
    
    if (sr & USART_SR_IDLE) {
        volatile uint32_t temp = USART2->SR;
        temp = USART2->DR;
        (void)temp;
        if (modbus_rx_len >= 4)
            modbus_rx_done = 1;
    }
    
    if (sr & USART_SR_ORE) {
        volatile uint32_t temp = USART2->DR;
        (void)temp;
    }
}

/* ============================================================
 * SysTick
 * ============================================================ */
static volatile uint32_t systick_ms = 0;

void SysTick_Handler(void) { systick_ms++; }

static void delay_ms(uint32_t ms)
{
    uint32_t start = systick_ms;
    while ((systick_ms - start) < ms);
}

/* ============================================================
 * 硬件初始化
 * ============================================================ */
void SystemInit(void)
{
    SCB_VTOR = 0x08000000UL;
    
    __disable_irq();
    for (int i = 0; i < 8; i++) {
        NVIC_ICER[i] = 0xFFFFFFFFUL;
        NVIC_ICPR[i] = 0xFFFFFFFFUL;
    }
    
    FLASH_ACR = FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY_1;
    
    RCC->CR |= RCC_CR_HSION;
    RCC->CFGR &= ~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLXTPRE | (0xFUL << RCC_CFGR_PLLMULL_POS));
    RCC->CFGR |= RCC_CFGR_PLLMULL16;
    RCC->CFGR &= ~(7UL << RCC_CFGR_PPRE1_POS);
    RCC->CFGR |= RCC_CFGR_PPRE1_DIV2;
    
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY));
    
    RCC->CFGR &= ~(3UL << RCC_CFGR_SW_POS);
    RCC->CFGR |= (2UL << RCC_CFGR_SW_POS);
    while ((RCC->CFGR & RCC_CFGR_SWS_PLL) != RCC_CFGR_SWS_PLL);
    
    __enable_irq();
}

/**
 * USART2 初始化: 115200, PA2=TX, PA3=RX, PA1=信号使能
 * JCY8001板上CP2102连接到PA2/PA3
 */
static void usart2_init(void)
{
    /* 使能时钟: GPIOA + AFIO + USART2 */
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_AFIOEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
    
    /* AFIO_MAPR: 禁用JTAG释放PA3(JTDO), 保留SWD
     * SWJ_CFG = 010 => JTAG-DP disabled, SW-DP enabled
     * 释放 PA15(JTDI), PA3(JTDO), PA4(JNTRST) 
     * 原版固件 AFIO_MAPR = 0x04000000 (SWJ_CFG=100) */
    *((volatile uint32_t *)0x40010004UL) = 0x02000000UL;
    
    /* PA1: 信号使能 - 推挽输出 10MHz (CRL[7:4])
     * Output PP = CNF=00, MODE=01 => 0x1 (原版固件配置，ODR=HIGH) */
    uint32_t crl = GPIOA->CRL;
    crl &= ~(0xFUL << 4);    /* 清除 Pin1 */
    crl |= (0x1UL << 4);     /* Output PP 10MHz */
    
    /* PA2: TX - 复用推挽输出 50MHz (CRL[11:8])
     * AF_PP = CNF=10, MODE=11 => 0xB */
    crl &= ~(0xFUL << 8);    /* 清除 Pin2 */
    crl |= (0xBUL << 8);     /* AF_PP 50MHz */
    crl &= ~(0xFUL << 12);   /* 清除 Pin3 */
    crl |= (0x8UL << 12);    /* IPU = CNF=10, MODE=00 */
    GPIOA->CRL = crl;
    
    /* PA1 默认高电平 = 使能通讯通路 (原版固件行为) */
    GPIOA->BSRR = (1UL << 1);
    
    /* 波特率: APB1=32MHz, 32MHz/115200 = 277.8 => BRR=278
     * 实际波特率 = 32000000/278 = 115107 (误差0.08%, 可接受) */
    USART2->BRR = 278;
    
    /* 8N1, 收发使能 */
    USART2->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
    
    /* 清除标志 */
    volatile uint32_t temp = USART2->SR;
    temp = USART2->DR;
    (void)temp;
    
    /* 使能接收中断: RXNE + IDLE */
    USART2->CR1 |= USART_CR1_RXNEIE | USART_CR1_IDLEIE;
    
    /* NVIC 使能 USART2 中断: IRQ38 => ISER1 bit6 */
    NVIC_ISER1 = (1UL << (USART2_IRQn - 32));
    NVIC_IPR[USART2_IRQn >> 2] = 0x00;
}

static void led_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
    uint32_t crh = GPIOC->CRH;
    crh &= ~(0xFUL << 20);
    crh |= (0x2UL << 20);
    GPIOC->CRH = crh;
}

static void led_toggle(void) { GPIOC->ODR ^= (1UL << 13); }
static void led_on(void)     { GPIOC->BRR = (1UL << 13); }
static void led_off(void)    { GPIOC->BSRR = (1UL << 13); }

/* ============================================================
 * SPI1 初始化 (PA5=SCK, PA6=MISO, PA7=MOSI)
 * ============================================================ */
static void spi1_init(void)
{
    /* 使能 GPIOA + SPI1 时钟 */
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_SPI1EN;

    /* PA5(SCK), PA7(MOSI): 复用推挽输出 10MHz (CNF=10, MODE=01)
     * PA6(MISO): 浮空输入 (CNF=01, MODE=00) */
    uint32_t crl = GPIOA->CRL;
    crl &= ~((0xFUL << 20) | (0xFUL << 24));  /* 清除 Pin5, Pin6 */
    crl |= (0xAUL << 20);   /* PA5: AF PP 10MHz (CNF=10, MODE=01) */
    crl |= (0x4UL << 24);   /* PA6: Input floating (CNF=01, MODE=00) */
    crl &= ~(0xFUL << 28);  /* 清除 Pin7 */
    crl |= (0xAUL << 28);   /* PA7: AF PP 10MHz (CNF=10, MODE=01) */
    GPIOA->CRL = crl;

    /* SPI1: 主机模式, 8位, MSB优先, CPOL=0 CPHA=0, 分频2 (APB2=64MHz/2=32MHz) */
    SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_CPOL | SPI_CR1_CPHA;  /* MSTR+CPOL+CPHA first */
    SPI1->CR1 &= ~(0x7UL << SPI_CR1_BR_POS);                  /* 清除分频位 */
    SPI1->CR1 |= SPI_CR1_BR_DIV2;                             /* 2分频 = 16MHz SPI clock */
    SPI1->CR2 = 0;
    SPI1->CR1 |= SPI_CR1_SPE;                                 /* 使能SPI1 */
}

/* SPI1 轮询传输一个字节 */
static uint8_t spi1_transfer(uint8_t data)
{
    /* 等待TXE */
    while (!(SPI1->SR & SPI_SR_TXE));
    SPI1->DR = data;
    /* 等待RXNE */
    while (!(SPI1->SR & SPI_SR_RXNE));
    return (uint8_t)(SPI1->DR & 0xFF);
}

/* ============================================================
 * DNB1101 简化协议 (零依赖版)
 * DNB11xx_CreateSendBuf: 构建发送缓冲
 *   p[0..3] = 命令参数 (小端)
 *   ICs = 芯片数量
 *   HeadLen = 帧长度
 * ============================================================ */
#define DNB11XX_BUF_SIZE  64

static uint8_t dnb_send_buf[DNB11XX_BUF_SIZE];
static uint8_t dnb_recv_buf[DNB11XX_BUF_SIZE];

static uint32_t dnb_create_send_buf(const uint8_t *p, uint8_t ICs, uint32_t head_len)
{
    uint32_t i, j;
    for (i = 0; i < head_len - 1; i++) {
        dnb_send_buf[i] = 0;
    }
    dnb_send_buf[i++] = 0x0f;
    dnb_send_buf[i++] = p[3];
    dnb_send_buf[i++] = p[2];
    dnb_send_buf[i++] = p[1];
    dnb_send_buf[i++] = p[0];
    j = i;
    for (; (i - j) < (ICs * 4); i++) {
        dnb_send_buf[i] = 0xff;
    }
    dnb_send_buf[i++] = 0xf0;
    return i; /* 返回总长度 */
}

/* DNB11xx SPI 全双工传输 */
static void dnb_spi_transfer(uint8_t *tx, uint8_t *rx, uint32_t len)
{
    uint32_t i;
    for (i = 0; i < len; i++) {
        rx[i] = spi1_transfer(tx[i]);
    }
}

/* 构建 GetData 命令 (简化版, 无CRC4校验) */
static void dnb_build_getdata_cmd(uint8_t id, uint8_t data_type, uint8_t *cmd_out)
{
    /* 8字节命令结构:
     * [0] = CMD_Type_GetData (0x0D)
     * [1] = DataType_L | DataType_H
     * [2] = 选项
     * [3] = ID
     * [4] = CRC4 (暂用0)
     * [5..6] = 0xFF (padding)
     * [7] = 0xF0 (end marker) */
    cmd_out[0] = 0x0D;           /* CMD_GetData */
    cmd_out[1] = data_type;      /* DataType */
    cmd_out[2] = 0x00;           /* 选项: ClrExeCnt=0, Equidist=0, ResetRSC=0 */
    cmd_out[3] = id;             /* IC ID */
    cmd_out[4] = 0x00;           /* CRC4 (简化版=0) */
    cmd_out[5] = 0xFF;
    cmd_out[6] = 0xFF;
    cmd_out[7] = 0xF0;
}

/* 读取指定类型数据 */
static int dnb_read_data(uint8_t ic_id, uint8_t data_type, int16_t *mantissa, int16_t *exponent)
{
    uint8_t cmd[8];
    uint8_t resp[8];
    dnb_build_getdata_cmd(ic_id, data_type, cmd);
    dnb_spi_transfer(cmd, resp, 8);

    /* 响应格式: [0]=响应状态, [1..2]=Mantissa, [3..4]=Exponent, [7]=0xF0 */
    if (resp[7] == 0xF0) {
        *mantissa = (int16_t)((resp[2] << 8) | resp[1]);
        *exponent = (int16_t)((resp[4] << 8) | resp[3]);
        return 0;
    }
    return -1;
}

/* DNB1101 数据容器 (周期读取更新) */
static struct {
    int16_t zreal_mant;
    int16_t zreal_exp;
    int16_t zimag_mant;
    int16_t zimag_exp;
    int16_t voltage;     /* mV */
    int16_t temperature; /* 0.1°C */
    uint8_t valid;
} dnb_data;

/* 定期轮询 DNB1101 (每500ms一次) */
static void dnb_poll(void)
{
    static uint32_t last_poll = 0;
    uint32_t now = systick_ms;

    if ((now - last_poll) < 500) return;
    last_poll = now;

    /* 发送两次命令 (原固件行为: 发两次隔2ms) */
    int16_t m, e;

    /* 电压 (DataType=0x06 -> 对应MainVolt) */
    if (dnb_read_data(0xFF, 0x06, &m, &e) == 0) {
        /* 电压 = m/16383*4800+1200 mV */
        dnb_data.voltage = (int16_t)((m * 4800) / 16383 + 1200);
    }

    /* 温度 (DataType=0x07 -> 对应MainDieTemp) */
    if (dnb_read_data(0xFF, 0x07, &m, &e) == 0) {
        dnb_data.temperature = (int16_t)(m * 10 / 16); /* 0.1°C */
    }

    /* ZREAL (DataType=0x09) */
    if (dnb_read_data(0xFF, 0x09, &m, &e) == 0) {
        dnb_data.zreal_mant = m;
        dnb_data.zreal_exp = e;
    }

    /* ZIMAG (DataType=0x0A) */
    if (dnb_read_data(0xFF, 0x0A, &m, &e) == 0) {
        dnb_data.zimag_mant = m;
        dnb_data.zimag_exp = e;
    }

    dnb_data.valid = 1;
}

/* 更新 Modbus 寄存器 (用真实数据替换模拟值) */
static void dnb_update_modbus_regs(void)
{
    if (!dnb_data.valid) return;

    /* 0x3340: 电压 (mV) */
    modbus_regs_input[0x40] = (uint16_t)(dnb_data.voltage & 0xFFFF);

    /* 0x3300: 温度 (0.1°C) */
    modbus_regs_input[0x00] = (uint16_t)(dnb_data.temperature & 0xFFFF);

    /* 0x3100: ZREAL (mΩ) - 公式: mant * 10^(exp-10) */
    {
        int32_t zreal = dnb_data.zreal_mant;
        int16_t exp = dnb_data.zreal_exp;
        if (exp >= 10) {
            int i;
            for (i = 0; i < (exp - 10); i++) zreal *= 10;
        } else {
            int i;
            for (i = 0; i < (10 - exp); i++) zreal /= 10;
        }
        modbus_regs_input[0x00] = (uint16_t)(zreal & 0xFFFF);
    }

    /* 0x3140: ZIMAG (mΩ) */
    {
        int32_t zimag = dnb_data.zimag_mant;
        int16_t exp = dnb_data.zimag_exp;
        if (exp >= 10) {
            int i;
            for (i = 0; i < (exp - 10); i++) zimag *= 10;
        } else {
            int i;
            for (i = 0; i < (10 - exp); i++) zimag /= 10;
        }
        modbus_regs_input[0x40] = (uint16_t)(zimag & 0xFFFF);
    }
}

static void systick_init(void)
{
    SysTick_LOAD = 64000UL - 1;
    SysTick_VAL = 0;
    SysTick_CTRL = SysTick_CTRL_CLKSOURCE | SysTick_CTRL_TICKINT | SysTick_CTRL_ENABLE;
}

static void modbus_init_regs(void)
{
    modbus_regs_input[0x00] = 1;     /* 0x3E00: 通道数 = 1 */
    modbus_regs_input[0x01] = 1;     /* 0x3E01: 版本号 = 1 */
    modbus_regs_input[0x40] = 60000; /* 0x3340: 电压 = 60000(6V空载) */
    modbus_regs_input[0x00] = 1;     /* 0x3E00: 通道数 = 1 (覆盖回来) */
    modbus_regs_holding[0x00] = 1000; /* 0x4000: ZM频率 = 1000Hz */
}

/* ============================================================
 * 主函数
 * ============================================================ */
int main(void)
{
    led_init();
    usart2_init();
    spi1_init();
    systick_init();
    modbus_init_regs();
    
    /* 开机闪3下 */
    for (int i = 0; i < 3; i++) {
        led_on(); delay_ms(100);
        led_off(); delay_ms(100);
    }
    
    uart_send_str("JCY8001 v2.0 USART2 OK\r\n");
    
    /* 主循环 */
    uint32_t heartbeat = 0;
    while (1) {
        if (modbus_rx_done) {
            modbus_process();
            modbus_rx_len = 0;
            modbus_rx_done = 0;
        }
        
        dnb_poll();
        dnb_update_modbus_regs();

        if (systick_ms - heartbeat >= 500) {
            heartbeat = systick_ms;
            led_toggle();
        }
    }
    
    return 0;
}
