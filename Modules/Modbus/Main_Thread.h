/**
 * Main_Thread.h - 主线程接口
 * 简化版，仅包含 Modbus 需要的接口
 */

#ifndef __MAIN_THREAD_H
#define __MAIN_THREAD_H

#include <stdint.h>

// Modbus 回调函数（由 Modbus 调用，在 main.c 中实现）
void Modbus_Callback(uint8_t *data, uint16_t length);

#endif /* __MAIN_THREAD_H */
