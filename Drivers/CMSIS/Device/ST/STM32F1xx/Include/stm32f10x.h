/**
 * STM32F10x 标准外设库头文件包装器
 * 包含标准外设库的所有定义
 */

#ifndef __STM32F10X_H
#define __STM32F10X_H

#include "stm32f1xx.h"
#include "stm32f103xe.h"

// 兼容旧版类型定义
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef int32_t s32;
typedef int16_t s16;
typedef int8_t s8;

// 时钟配置
#define HSE_VALUE        ((uint32_t)8000000)   // 外部晶振 8MHz
#define HSI_VALUE        ((uint32_t)8000000)   // 内部时钟 8MHz
#define HSE_STARTUP_TIMEOUT   ((uint16_t)0x5000)

// DMA 寄存器兼容
#define DMA_CCR1_EN      DMA_CCR_EN

// 断言宏
#define assert_param(expr) ((void)0)

// 标准外设库头文件
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_dma.h"
#include "stm32f10x_flash.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_adc.h"
#include "stm32f10x_i2c.h"
#include "stm32f10x_spi.h"
#include "misc.h"

#endif /* __STM32F10X_H */
