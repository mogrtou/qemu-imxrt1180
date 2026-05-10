/**
 * FreeRTOSConfig.h — Cortex-M7 FreeRTOS 配置
 *
 * 目标: i.MX RT1180 Cortex-M7 (ARMv7E-M)
 * 时钟: SysTick 来自 ARMv7-M 内核 (不可抢占优先级)
 *
 * 由于当前为 bare-metal Phase 1, FreeRTOS 尚未运行,
 * 此配置文件为 Phase 2 (lwIP + FreeRTOS 集成) 做准备。
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* ==========================================================================
 * 硬件相关
 * ========================================================================== */
#define configCPU_CLOCK_HZ              (600000000UL)   /* 600 MHz */
#define configTICK_RATE_HZ              (1000)          /* 1 kHz tick */
#define configMAX_PRIORITIES            (8)             /* 0-7 优先级 */
#define configMINIMAL_STACK_SIZE        (128)           /* 最小堆栈 (字) */
#define configTOTAL_HEAP_SIZE           (32 * 1024)     /* 32KB 堆 */
#define configMAX_TASK_NAME_LEN         (16)

/* ==========================================================================
 * FreeRTOS 版本兼容
 * ========================================================================== */
#define configENABLE_BACKWARD_COMPATIBILITY  0   /* 关闭旧版兼容 */
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0 /* M7 不使用硬件 CLZ */

/* ==========================================================================
 * 调度配置
 * ========================================================================== */
#define configUSE_PREEMPTION            1               /* 抢占式 */
#define configUSE_TIME_SLICING          1               /* 时间片 */
#define configTICK_TYPE_WIDTH_IN_BITS   TICK_TYPE_WIDTH_32_BITS
#define configIDLE_SHOULD_YIELD         1
#define configUSE_TASK_NOTIFICATIONS    1

/* ==========================================================================
 * 同步原语
 * ========================================================================== */
#define configUSE_MUTEXES               1
#define configUSE_RECURSIVE_MUTEXES     1
#define configUSE_COUNTING_SEMAPHORES   1
#define configUSE_QUEUE_SETS            0

/* ==========================================================================
 * 内存分配
 * ========================================================================== */
#define configSUPPORT_STATIC_ALLOCATION 0
#define configSUPPORT_DYNAMIC_ALLOCATION 1

/* ==========================================================================
 * 钩子函数
 * ========================================================================== */
#define configUSE_IDLE_HOOK             0
#define configUSE_TICK_HOOK             0
#define configCHECK_FOR_STACK_OVERFLOW  0   /* Phase 1 关闭, Phase 2 启用 */
#define configUSE_MALLOC_FAILED_HOOK    0

/* ==========================================================================
 * 软件定时器
 * ========================================================================== */
#define configUSE_TIMERS                1
#define configTIMER_TASK_PRIORITY       (configMAX_PRIORITIES - 1)
#define configTIMER_QUEUE_LENGTH        8
#define configTIMER_TASK_STACK_DEPTH    (configMINIMAL_STACK_SIZE)

/* ==========================================================================
 * 内核中断优先级
 * ========================================================================== */
#define configKERNEL_INTERRUPT_PRIORITY          (7 << 4)   /* 最低优先级 (255) */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY     (5 << 4)   /* 允许系统调用 (191+) */

/* ==========================================================================
 * 可选功能
 * ========================================================================== */
#define INCLUDE_vTaskPrioritySet         1
#define INCLUDE_uxTaskPriorityGet        1
#define INCLUDE_vTaskDelete              1
#define INCLUDE_vTaskSuspend             1
#define INCLUDE_vTaskDelayUntil          1
#define INCLUDE_vTaskDelay               1
#define INCLUDE_xTaskGetSchedulerState   1
#define INCLUDE_xTaskGetCurrentTaskHandle 1
#define INCLUDE_eTaskGetState            1

/* ==========================================================================
 * 运行时统计 (可选)
 * ========================================================================== */
#define configGENERATE_RUN_TIME_STATS    0
#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS()
#define portGET_RUN_TIME_COUNTER_VALUE() 0

/* ==========================================================================
 * lwIP 集成需要
 * ========================================================================== */
#define configUSE_TRACE_FACILITY         1

/* ==========================================================================
 * 核心
 * ========================================================================== */
#define configENABLE_FPU                0   /* 不使用 FPU (mfloat-abi=soft) */

/* ==========================================================================
 * ARM Cortex-M 特定
 * ========================================================================== */
/* NVIC 寄存器定义 — 由官方的 portmacro.h 提供, 这里不重复定义 */

#endif /* FREERTOS_CONFIG_H */
