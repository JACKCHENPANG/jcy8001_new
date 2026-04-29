/**
 * modbus.c - 简化版 Modbus 通讯
 * 仅保留核心通讯逻辑，不包含业务处理
 */

#include "Modbus.h"
#include "stdint.h"
#include "stdbool.h"
#include "cmsis_os.h"
#include "CRC.h"
#include <string.h>

// Modbus 从站地址
#define MODBUS_SLAVE_ADDR   1

// 接收缓冲区
#define RX_BUF_SIZE 256
static uint8_t RxBuffer[RX_BUF_SIZE];
static uint16_t RxCount = 0;

// 发送缓冲区
#define TX_BUF_SIZE 256
static uint8_t TxBuffer[TX_BUF_SIZE];

// 外部函数声明（在 Driver_USART1.c 中实现）
extern void USART1_Send(uint8_t *p, uint16_t Length, bool isWaiting);

/**
 * CRC16 计算
 */
static uint16_t Modbus_CRC16(uint8_t *data, uint16_t length) {
    return Modbus_CRC16_Lookup(data, length);
}

/**
 * 发送 Modbus 响应
 */
static void Modbus_SendResponse(uint8_t *data, uint16_t length) {
    // 添加 CRC
    uint16_t crc = Modbus_CRC16(data, length);
    data[length] = crc & 0xFF;
    data[length + 1] = (crc >> 8) & 0xFF;
    
    // 发送
    USART1_Send(data, length + 2, true);
}

/**
 * 处理 Modbus 请求
 */
static void Modbus_ProcessRequest(uint8_t *data, uint16_t length) {
    if (length < 4) return;  // 最小帧长度
    
    // 验证 CRC
    uint16_t received_crc = data[length - 2] | (data[length - 1] << 8);
    uint16_t calculated_crc = Modbus_CRC16(data, length - 2);
    if (received_crc != calculated_crc) return;
    
    // 验证从站地址
    if (data[0] != MODBUS_SLAVE_ADDR) return;
    
    uint8_t function = data[1];
    
    // 简单的响应示例
    switch (function) {
        case 0x03:  // 读保持寄存器
        case 0x06:  // 写单个寄存器
        case 0x10:  // 写多个寄存器
            // 回显请求（简化处理）
            Modbus_SendResponse(data, length - 2);
            break;
        default:
            // 异常响应：功能码不支持
            TxBuffer[0] = data[0];
            TxBuffer[1] = function | 0x80;
            TxBuffer[2] = 0x01;  // 非法功能码
            Modbus_SendResponse(TxBuffer, 3);
            break;
    }
}

/**
 * 串口接收回调
 */
void CallBack_FromUsart1(uint8_t *p, uint16_t length, uint8_t Usart_Sel) {
    if (Usart_Sel == 1) {
        Modbus_ProcessRequest(p, length);
    }
}

/**
 * Modbus 初始化
 */
void Modbus_Init(void) {
    RxCount = 0;
}

/**
 * Modbus 线程（FreeRTOS 任务）
 */
static void Modbus_Thread(const void *arg) {
    (void)arg;
    Modbus_Init();
    
    for (;;) {
        osDelay(100);  // 简单延时
    }
}

// FreeRTOS 任务定义
osThreadDef(Modbus_Thread, osPriorityNormal, 1, 512);

/**
 * 创建 Modbus 任务
 */
void Modbus_CreateTask(void) {
    osThreadCreate(osThread(Modbus_Thread), NULL);
}
