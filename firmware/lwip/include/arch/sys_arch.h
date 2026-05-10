/**
 * arch/sys_arch.h — lwIP OS 移植层类型定义 + 编译标志
 *
 * 所有 OS 函数声明由 lwIP 内部的 sys.h 提供。
 * 实现在 arch/sys_arch.c (空函数, bare-metal Phase 1)。
 */

#ifndef LWIP_ARCH_SYS_ARCH_H
#define LWIP_ARCH_SYS_ARCH_H

#include "lwip/err.h"

/* ── 类型 ── */
typedef void * sys_sem_t;
typedef void * sys_mutex_t;
typedef void * sys_mbox_t;
typedef void * sys_thread_t;
typedef unsigned int sys_prot_t;

#define SYS_SEM_NULL    NULL
#define SYS_MUTEX_NULL  NULL
#define SYS_MBOX_NULL   NULL

/* ── 编译标志 ── */
#define LWIP_NO_CTYPE_H     1
#define LWIP_NO_STDINT_H    1
#define LWIP_NOASSERT       1

/* ── 临界区 (FreeRTOS 实现) ── */
sys_prot_t sys_arch_protect(void);
void sys_arch_unprotect(sys_prot_t pval);

/* ── OS 函数声明 ── */
err_t sys_mbox_trypost_fromisr(sys_mbox_t *mbox, void *msg);
sys_thread_t sys_thread_new(const char *name,
                   void (*thread_func)(void *), void *arg,
                   int stacksize, int prio);
void sys_mbox_set_invalid(sys_mbox_t *mbox);
int sys_mbox_valid(sys_mbox_t *mbox);
void sys_sem_set_invalid(sys_sem_t *sem);
int sys_sem_valid(sys_sem_t *sem);
void sys_mutex_set_invalid(sys_mutex_t *mutex);
int sys_mutex_valid(sys_mutex_t *mutex);

#endif /* LWIP_ARCH_SYS_ARCH_H */
