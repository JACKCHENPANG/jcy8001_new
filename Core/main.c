/**
 * JCY8001 串口测试 - 最小化版本
 * 测试 USART1 和 USART2，确认哪个串口连接到 USB-TTL
 */

#include "stm32f10x.h"
#include <string.h>

// 接收缓冲区
#define RX_BUF_SIZE 256
static uint8_t RxBuffer[RX_BUF_SIZE];
static volatile uint16_t RxCount = 0;
static volatile uint8_t RxComplete = 0;

// 外部函数
extern void RCC_Configuration(void);

/**
 * 简单延时
 */
void delay_ms(uint32_t ms)
{
    for (uint32_t i = 0; i < ms * 8000; i++) {
        __NOP();
    }
}

/**
 * USART1 初始化 (PA9=TX, PA10=RX)
 */
void USART1_Test_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    USART_InitTypeDef USART_InitStruct;
    NVIC_InitTypeDef NVIC_InitStruct;
    
    // 使能时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1 | RCC_APB2Periph_AFIO, ENABLE);
    
    // PA9: TX - 复用推挽输出
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    // PA10: RX - 上拉输入
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    // USART 配置
    USART_InitStruct.USART_BaudRate = 115200;
    USART_InitStruct.USART_WordLength = USART_WordLength_8b;
    USART_InitStruct.USART_StopBits = USART_StopBits_1;
    USART_InitStruct.USART_Parity = USART_Parity_No;
    USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStruct.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(USART1, &USART_InitStruct);
    
    // 清除标志
    volatile uint32_t temp = USART1->SR;
    temp = USART1->DR;
    (void)temp;
    
    // 使能接收中断
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
    USART_ITConfig(USART1, USART_IT_IDLE, ENABLE);
    
    // NVIC 配置
    NVIC_InitStruct.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);
    
    // 使能 USART
    USART_Cmd(USART1, ENABLE);
}

/**
 * USART2 初始化 (PA2=TX, PA3=RX)
 */
void USART2_Test_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    USART_InitTypeDef USART_InitStruct;
    NVIC_InitTypeDef NVIC_InitStruct;
    
    // 使能时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
    
    // PA2: TX - 复用推挽输出
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    // PA3: RX - 上拉输入
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    // USART 配置
    USART_InitStruct.USART_BaudRate = 115200;
    USART_InitStruct.USART_WordLength = USART_WordLength_8b;
    USART_InitStruct.USART_StopBits = USART_StopBits_1;
    USART_InitStruct.USART_Parity = USART_Parity_No;
    USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStruct.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(USART2, &USART_InitStruct);
    
    // 清除标志
    volatile uint32_t temp = USART2->SR;
    temp = USART2->DR;
    (void)temp;
    
    // 使能接收中断
    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
    USART_ITConfig(USART2, USART_IT_IDLE, ENABLE);
    
    // NVIC 配置
    NVIC_InitStruct.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStruct.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);
    
    // 使能 USART
    USART_Cmd(USART2, ENABLE);
}

/**
 * USART1 发送字符串
 */
void USART1_SendString(const char *str)
{
    while (*str) {
        while (!(USART1->SR & USART_SR_TXE));
        USART1->DR = *str++;
    }
    while (!(USART1->SR & USART_SR_TC));
}

/**
 * USART2 发送字符串
 */
void USART2_SendString(const char *str)
{
    while (*str) {
        while (!(USART2->SR & USART_SR_TXE));
        USART2->DR = *str++;
    }
    while (!(USART2->SR & USART_SR_TC));
}

/**
 * USART1 中断处理
 */
void USART1_IRQHandler(void) __attribute__((interrupt("IRQ")));
void USART1_IRQHandler(void)
{
    volatile uint32_t temp;
    
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
        RxBuffer[RxCount++] = USART1->DR;
        if (RxCount >= RX_BUF_SIZE) RxCount = 0;
    }
    
    if (USART_GetITStatus(USART1, USART_IT_IDLE) != RESET) {
        temp = USART1->SR;
        temp = USART1->DR;
        (void)temp;
        RxComplete = 1;
    }
}

/**
 * USART2 中断处理
 */
void USART2_IRQHandler(void) __attribute__((interrupt("IRQ")));
void USART2_IRQHandler(void)
{
    volatile uint32_t temp;
    
    if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET) {
        RxBuffer[RxCount++] = USART2->DR;
        if (RxCount >= RX_BUF_SIZE) RxCount = 0;
    }
    
    if (USART_GetITStatus(USART2, USART_IT_IDLE) != RESET) {
        temp = USART2->SR;
        temp = USART2->DR;
        (void)temp;
        RxComplete = 2;  // 标记 USART2 接收完成
    }
}

/**
 * 主函数
 */
int main(void)
{
    // 系统时钟配置（SystemInit 已调用 RCC_Configuration）
    
    // 等待稳定
    delay_ms(100);
    
    // 初始化两个串口
    USART1_Test_Init();
    USART2_Test_Init();
    
    // 发送启动消息
    USART1_SendString("USART1 Ready (PA9/PA10)\r\n");
    USART2_SendString("USART2 Ready (PA2/PA3)\r\n");
    
    // 主循环
    while (1) {
        // 检查 USART1 接收
        if (RxComplete == 1) {
            USART1_SendString("USART1 RX: ");
            for (uint16_t i = 0; i < RxCount; i++) {
                while (!(USART1->SR & USART_SR_TXE));
                USART1->DR = RxBuffer[i];
            }
            USART1_SendString("\r\n");
            RxCount = 0;
            RxComplete = 0;
        }
        
        // 检查 USART2 接收
        if (RxComplete == 2) {
            USART2_SendString("USART2 RX: ");
            for (uint16_t i = 0; i < RxCount; i++) {
                while (!(USART2->SR & USART_SR_TXE));
                USART2->DR = RxBuffer[i];
            }
            USART2_SendString("\r\n");
            RxCount = 0;
            RxComplete = 0;
        }
        
        // 每秒发送心跳
        delay_ms(1000);
        USART1_SendString("USART1 Heartbeat\r\n");
        USART2_SendString("USART2 Heartbeat\r\n");
    }
    
    return 0;
}
