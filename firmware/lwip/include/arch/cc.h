/**
 * arch/cc.h — lwIP Cortex-M7 bare-metal 编译器适配
 *
 * 定义 lwIP 所需的类型、字节序、printf 风格宏。
 * bare-metal 环境: 无 OS, 无标准库。
 */

#ifndef LWIP_ARCH_CC_H
#define LWIP_ARCH_CC_H

#include <stdint.h>

/* ── 禁用不存在的系统头文件 (bare-metal) ── */
#define LWIP_NO_INTTYPES_H  1

/* ── 基础类型 ── */
typedef uint8_t   u8_t;
typedef int8_t    s8_t;
typedef uint16_t  u16_t;
typedef int16_t   s16_t;
typedef uint32_t  u32_t;
typedef int32_t   s32_t;

/* ── 平台相关类型 ── */
typedef uintptr_t mem_ptr_t;

/* ── 结构体打包 ── */
#define PACK_STRUCT_STRUCT  __attribute__((packed))
#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_END

/* ── printf 格式宏 ── */
#define U16_F  "hu"
#define S16_F  "hd"
#define X16_F  "hx"
#define U32_F  "lu"
#define S32_F  "ld"
#define X32_F  "lx"
#define SZT_F  "lu"

/* ── 字节序 (Cortex-M7 为小端) ── */
#define BYTE_ORDER  LITTLE_ENDIAN

/* ── 诊断 / 断言 — 由 lwipopts.h 统一控制 ── */
#ifndef LWIP_PLATFORM_DIAG
#define LWIP_PLATFORM_DIAG(x)  do { } while (0)
#endif
#ifndef LWIP_PLATFORM_ASSERT
#define LWIP_PLATFORM_ASSERT(x)  do { } while (0)
#endif

#endif /* LWIP_ARCH_CC_H */
