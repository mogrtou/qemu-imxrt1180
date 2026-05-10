/**
 * arch/sys_arch.c — lwIP FreeRTOS 移植层 (M3)
 *
 * 所有函数签名严格匹配 lwIP/src/include/lwip/sys.h。
 * 使用 FreeRTOS 信号量/互斥锁/邮箱/线程实现 OS 适配。
 *
 * 从 bare-metal 空实现升级为完整 FreeRTOS 实现。
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "arch/sys_arch.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include "lwip/err.h"

/* lwIP sys_arch timeout 常量 (与 FreeRTOS 配合) */
#ifndef SYS_ARCH_TIMEOUT
#define SYS_ARCH_TIMEOUT  0xFFFFFFFFUL
#endif

/* ==========================================================================
 * 互斥锁 — 基于 FreeRTOS 递归互斥锁
 * ========================================================================== */

int sys_mutex_new(sys_mutex_t *mutex)
{
    SemaphoreHandle_t m = xSemaphoreCreateRecursiveMutex();
    if (!m) return -1;
    *mutex = (sys_mutex_t)m;
    return 0;
}

void sys_mutex_lock(sys_mutex_t *mutex)
{
    if (*mutex) {
        xSemaphoreTakeRecursive((SemaphoreHandle_t)*mutex, portMAX_DELAY);
    }
}

void sys_mutex_unlock(sys_mutex_t *mutex)
{
    if (*mutex) {
        xSemaphoreGiveRecursive((SemaphoreHandle_t)*mutex);
    }
}

void sys_mutex_free(sys_mutex_t *mutex)
{
    if (*mutex) {
        vSemaphoreDelete((SemaphoreHandle_t)*mutex);
        *mutex = NULL;
    }
}

int sys_mutex_valid(sys_mutex_t *mutex)
{
    return (*mutex != SYS_MUTEX_NULL) ? 1 : 0;
}

void sys_mutex_set_invalid(sys_mutex_t *mutex)
{
    *mutex = SYS_MUTEX_NULL;
}

/* ==========================================================================
 * 邮箱 — 基于 FreeRTOS Queue (void*)
 * ========================================================================== */

int sys_mbox_new(sys_mbox_t *mbox, int size)
{
    QueueHandle_t q = xQueueCreate((UBaseType_t)size, sizeof(void *));
    if (!q) return -1;
    *mbox = (sys_mbox_t)q;
    return 0;
}

void sys_mbox_post(sys_mbox_t *mbox, void *msg)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (*mbox) {
        xQueueSendFromISR((QueueHandle_t)*mbox, &msg, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

int sys_mbox_trypost(sys_mbox_t *mbox, void *msg)
{
    BaseType_t ret;
    if (!*mbox) return -1;
    ret = xQueueSend((QueueHandle_t)*mbox, &msg, 0);
    return (ret == pdTRUE) ? 0 : -1;
}

/**
 * sys_mbox_trypost_fromisr — 从 ISR 上下文向邮箱投递消息
 * lwIP tcpip.c 在回调中调用此函数
 */
err_t sys_mbox_trypost_fromisr(sys_mbox_t *mbox, void *msg)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    BaseType_t ret;
    if (!*mbox) return ERR_VAL;
    ret = xQueueSendFromISR((QueueHandle_t)*mbox, &msg, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    return (ret == pdTRUE) ? ERR_OK : ERR_MEM;
}

void sys_mbox_free(sys_mbox_t *mbox)
{
    if (*mbox) {
        vQueueDelete((QueueHandle_t)*mbox);
        *mbox = NULL;
    }
}

int sys_mbox_valid(sys_mbox_t *mbox)
{
    return (*mbox != SYS_MBOX_NULL) ? 1 : 0;
}

void sys_mbox_set_invalid(sys_mbox_t *mbox)
{
    *mbox = SYS_MBOX_NULL;
}

uint32_t sys_arch_mbox_fetch(sys_mbox_t *mbox, void **msg, uint32_t timeout)
{
    BaseType_t ret;
    void *p;

    if (!*mbox) {
        if (msg) *msg = NULL;
        return SYS_ARCH_TIMEOUT;
    }

    ret = xQueueReceive((QueueHandle_t)*mbox, &p,
                        (timeout == 0) ? portMAX_DELAY : pdMS_TO_TICKS(timeout));
    if (ret == pdTRUE) {
        if (msg) *msg = p;
        return 0;
    }
    if (msg) *msg = NULL;
    return SYS_ARCH_TIMEOUT;
}

uint32_t sys_arch_mbox_tryfetch(sys_mbox_t *mbox, void **msg)
{
    return sys_arch_mbox_fetch(mbox, msg, 1);
}

/* ==========================================================================
 * 信号量 — 基于 FreeRTOS Counting Semaphore
 * ========================================================================== */

int sys_sem_new(sys_sem_t *sem, uint8_t count)
{
    SemaphoreHandle_t s = xSemaphoreCreateCounting(0xFF, count);
    if (!s) return -1;
    *sem = (sys_sem_t)s;
    return 0;
}

void sys_sem_signal(sys_sem_t *sem)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (*sem) {
        xSemaphoreGiveFromISR((SemaphoreHandle_t)*sem, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

void sys_sem_free(sys_sem_t *sem)
{
    if (*sem) {
        vSemaphoreDelete((SemaphoreHandle_t)*sem);
        *sem = NULL;
    }
}

int sys_sem_valid(sys_sem_t *sem)
{
    return (*sem != SYS_SEM_NULL) ? 1 : 0;
}

void sys_sem_set_invalid(sys_sem_t *sem)
{
    *sem = SYS_SEM_NULL;
}

uint32_t sys_arch_sem_wait(sys_sem_t *sem, uint32_t timeout)
{
    BaseType_t ret;
    if (!*sem) return SYS_ARCH_TIMEOUT;
    ret = xSemaphoreTake((SemaphoreHandle_t)*sem,
                         (timeout == 0) ? portMAX_DELAY : pdMS_TO_TICKS(timeout));
    return (ret == pdTRUE) ? 0 : SYS_ARCH_TIMEOUT;
}

/* ==========================================================================
 * 线程 — 基于 FreeRTOS Task
 * ========================================================================== */

sys_thread_t sys_thread_new(const char *name,
                   void (*thread_func)(void *), void *arg,
                   int stacksize, int prio)
{
    TaskHandle_t h;
    BaseType_t ret;

    ret = xTaskCreate((TaskFunction_t)thread_func,
                      name,
                      (configSTACK_DEPTH_TYPE)stacksize,
                      arg,
                      (UBaseType_t)prio,
                      &h);
    if (ret != pdPASS) {
        return (sys_thread_t)NULL;
    }
    return (sys_thread_t)h;
}

/* ==========================================================================
 * 系统时间 — 基于 FreeRTOS tick
 * ========================================================================== */

uint32_t sys_now(void)
{
    /* FreeRTOS tick → ms: tick * 1000 / configTICK_RATE_HZ */
    return (uint32_t)(xTaskGetTickCount() * 1000 / configTICK_RATE_HZ);
}

/* ==========================================================================
 * sys_init — lwIP OS 层初始化 (由 lwip_init 调用)
 * ========================================================================== */

void sys_init(void)
{
    /* FreeRTOS 在 vTaskStartScheduler() 前已初始化完成 */
}

/* ==========================================================================
 * 临界区 — 使用 FreeRTOS 关全局中断
 * lwIP 要求 SYS_ARCH_PROTECT/UNPROTECT 嵌套安全
 * ========================================================================== */

sys_prot_t sys_arch_protect(void)
{
    taskENTER_CRITICAL();
    return (sys_prot_t)1;
}

void sys_arch_unprotect(sys_prot_t pval)
{
    (void)pval;
    taskEXIT_CRITICAL();
}
