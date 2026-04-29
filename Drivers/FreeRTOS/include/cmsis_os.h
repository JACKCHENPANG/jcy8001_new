/**
 * CMSIS-RTOS 兼容层
 * 适配 FreeRTOS V11.x
 */

#ifndef __CMSIS_OS_H
#define __CMSIS_OS_H

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include "timers.h"

// CMSIS-RTOS 状态码
#define osOK                    0
#define osEventSignal          0x08
#define osEventMessage         0x10
#define osEventMail            0x20
#define osEventTimeout         0x40
#define osErrorParameter       0x80
#define osErrorResource        0x81
#define osErrorTimeoutResource 0xC1
#define osErrorISR             0x82
#define osErrorISRRecursive    0x83
#define osErrorPriority        0x84
#define osErrorNoMemory        0x85
#define osErrorValue           0x86
#define osErrorOS              0xFF
#define osStatusError         -0x7F
#define osWaitForever          0xFFFFFFFF

// 状态类型
typedef int osStatus;

// 事件结构
typedef struct {
    int status;
    union {
        uint32_t v;
        void *p;
    } value;
} osEvent;

// 线程优先级
typedef enum {
    osPriorityIdle          = -3,
    osPriorityLow           = -2,
    osPriorityBelowNormal   = -1,
    osPriorityNormal        =  0,
    osPriorityAboveNormal   = +1,
    osPriorityHigh          = +2,
    osPriorityRealtime      = +3,
    osPriorityError         = 0x84
} osPriority;

// 线程定义
typedef TaskHandle_t osThreadId;
typedef TimerHandle_t osTimerId;
typedef SemaphoreHandle_t osSemaphoreId;
typedef QueueHandle_t osMessageQId;
typedef QueueHandle_t osMailQId;

// 线程函数类型
typedef void (*os_pthread) (void const *argument);
typedef void (*os_ptimer) (void const *argument);

// 线程定义宏
typedef struct os_thread_def {
    os_pthread pthread;
    osPriority tpriority;
    uint32_t instances;
    uint32_t stacksize;
    const char *name;
} osThreadDef_t;

typedef struct os_timer_def {
    os_ptimer ptimer;
    const char *name;
} osTimerDef_t;

typedef struct os_semaphore_def {
    const char *name;
} osSemaphoreDef_t;

typedef struct os_messageQ_def {
    uint32_t queue_sz;
    uint32_t item_sz;
    const char *name;
} osMessageQDef_t;

// 线程创建
#define osThreadDef(name, priority, instances, stacksz) \
    osThreadDef_t os_thread_def_##name = { \
        (os_pthread)(name), (priority), (instances), (stacksz), #name \
    }
#define osThread(name) &os_thread_def_##name

static inline osThreadId osThreadCreate(const osThreadDef_t *thread_def, void *argument) {
    TaskHandle_t handle;
    BaseType_t ret = xTaskCreate(
        (TaskFunction_t)thread_def->pthread,
        thread_def->name,
        thread_def->stacksize / 4,  // 字转字
        argument,
        thread_def->tpriority + 3,  // CMSIS优先级转FreeRTOS
        &handle
    );
    return (ret == pdPASS) ? handle : NULL;
}

// 线程终止
static inline osStatus osThreadTerminate(osThreadId thread_id) {
    vTaskDelete(thread_id);
    return osOK;
}

// 线程延时
static inline osStatus osDelay(uint32_t millisec) {
    vTaskDelay(pdMS_TO_TICKS(millisec));
    return osOK;
}

// 信号量
#define osSemaphoreDef(name) \
    osSemaphoreDef_t os_semaphore_def_##name = { #name }
#define osSemaphore(name) &os_semaphore_def_##name

static inline osSemaphoreId osSemaphoreCreate(const osSemaphoreDef_t *semaphore_def, int32_t count) {
    (void)semaphore_def;
    return xSemaphoreCreateCounting(count, count);
}

static inline int32_t osSemaphoreWait(osSemaphoreId semaphore_id, uint32_t millisec) {
    TickType_t ticks = (millisec == osWaitForever) ? portMAX_DELAY : pdMS_TO_TICKS(millisec);
    return (xSemaphoreTake(semaphore_id, ticks) == pdTRUE) ? 0 : -1;
}

static inline osStatus osSemaphoreRelease(osSemaphoreId semaphore_id) {
    return (xSemaphoreGive(semaphore_id) == pdTRUE) ? osOK : osErrorOS;
}

// 消息队列
#define osMessageQDef(name, queue_sz, type) \
    osMessageQDef_t os_messageQ_def_##name = { (queue_sz), sizeof(type), #name }
#define osMessageQ(name) &os_messageQ_def_##name

static inline osMessageQId osMessageCreate(const osMessageQDef_t *queue_def, osThreadId thread_id) {
    (void)thread_id;
    return xQueueCreate(queue_def->queue_sz, queue_def->item_sz);
}

static inline osStatus osMessagePut(osMessageQId queue_id, uint32_t info, uint32_t millisec) {
    TickType_t ticks = (millisec == osWaitForever) ? portMAX_DELAY : pdMS_TO_TICKS(millisec);
    return (xQueueSend(queue_id, &info, ticks) == pdTRUE) ? osOK : osErrorOS;
}

static inline osEvent osMessageGet(osMessageQId queue_id, uint32_t millisec) {
    osEvent evt;
    TickType_t ticks = (millisec == osWaitForever) ? portMAX_DELAY : pdMS_TO_TICKS(millisec);
    uint32_t msg;
    if (xQueueReceive(queue_id, &msg, ticks) == pdTRUE) {
        evt.status = osEventMessage;
        evt.value.v = msg;
    } else {
        evt.status = osEventTimeout;
    }
    return evt;
}

// 定时器
#define osTimerDef(name, function) \
    osTimerDef_t os_timer_def_##name = { (os_ptimer)(function), #name }
#define osTimer(name) &os_timer_def_##name

typedef enum {
    osTimerOnce     = 0,
    osTimerPeriodic = 1
} os_timer_type;

static inline osTimerId osTimerCreate(const osTimerDef_t *timer_def, os_timer_type type, void *argument) {
    (void)argument;
    return xTimerCreate(
        timer_def->name,
        pdMS_TO_TICKS(1),  // 默认周期
        (type == osTimerPeriodic) ? pdTRUE : pdFALSE,
        NULL,
        (TimerCallbackFunction_t)timer_def->ptimer
    );
}

static inline osStatus osTimerStart(osTimerId timer_id, uint32_t millisec) {
    return (xTimerChangePeriod(timer_id, pdMS_TO_TICKS(millisec), 0) == pdPASS) ? osOK : osErrorOS;
}

static inline osStatus osTimerStop(osTimerId timer_id) {
    return (xTimerStop(timer_id, 0) == pdPASS) ? osOK : osErrorOS;
}

// 启动调度器
#define osKernelStart() vTaskStartScheduler()

#endif /* __CMSIS_OS_H */