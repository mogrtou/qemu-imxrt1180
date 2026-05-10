/**
 * mbedtls_config.h — mbedTLS 最小配置 (Phase 1)
 *
 * lwIP altcp TLS 层集成所需的最小 mbedTLS 功能集。
 * Phase 1 仅启用 TLS 客户端/服务器基础, Phase 2 启用完整证书管理。
 */

#ifndef MBEDTLS_CONFIG_H
#define MBEDTLS_CONFIG_H

/* ==========================================================================
 * 平台
 * ========================================================================== */
#define MBEDTLS_PLATFORM_C
#define MBEDTLS_PLATFORM_MEMORY
#define MBEDTLS_MEMORY_BUFFER_ALLOC_C
#define MBEDTLS_NO_PLATFORM_ENTROPY

/* ==========================================================================
 * 核心加密
 * ========================================================================== */
#define MBEDTLS_AES_C
#define MBEDTLS_CIPHER_C
#define MBEDTLS_CIPHER_MODE_CBC
#define MBEDTLS_CIPHER_MODE_CTR

#define MBEDTLS_MD5_C
#define MBEDTLS_SHA1_C
#define MBEDTLS_SHA224_C
#define MBEDTLS_SHA256_C

/* ==========================================================================
 * 非对称加密
 * ========================================================================== */
#define MBEDTLS_BIGNUM_C
#define MBEDTLS_ECP_C
#define MBEDTLS_ECP_DP_SECP256R1_ENABLED
#define MBEDTLS_ECDSA_C
#define MBEDTLS_ECDH_C

/* ==========================================================================
 * TLS / SSL
 * ========================================================================== */
#define MBEDTLS_SSL_TLS_C
#define MBEDTLS_SSL_CLI_C
#define MBEDTLS_SSL_SRV_C
#define MBEDTLS_SSL_PROTO_TLS1_2

/* ==========================================================================
 * X.509 证书
 * ========================================================================== */
#define MBEDTLS_X509_CRT_PARSE_C
#define MBEDTLS_X509_USE_C
#define MBEDTLS_PK_C
#define MBEDTLS_PK_PARSE_C
#define MBEDTLS_ASN1_PARSE_C
#define MBEDTLS_ASN1_WRITE_C

/* ==========================================================================
 * 随机数
 * ========================================================================== */
#define MBEDTLS_CTR_DRBG_C
#define MBEDTLS_ENTROPY_C
#define MBEDTLS_ENTROPY_HARDWARE_ALT   /* 使用 RT1180 TRNG (后续) */

/* ==========================================================================
 * PEM 解析
 * ========================================================================== */
#define MBEDTLS_PEM_PARSE_C
#define MBEDTLS_BASE64_C

/* ==========================================================================
 * 内存优化
 * ========================================================================== */
#define MBEDTLS_MPI_WINDOW_SIZE        1
#define MBEDTLS_MPI_MAX_SIZE           512
#define MBEDTLS_ECP_WINDOW_SIZE        2
#define MBEDTLS_ECP_FIXED_POINT_OPTIM  0
#define MBEDTLS_SSL_MAX_CONTENT_LEN    4096
#define MBEDTLS_SSL_IN_CONTENT_LEN     MBEDTLS_SSL_MAX_CONTENT_LEN
#define MBEDTLS_SSL_OUT_CONTENT_LEN    MBEDTLS_SSL_MAX_CONTENT_LEN

/* ==========================================================================
 * 调试
 * ========================================================================== */
#define MBEDTLS_DEBUG_C
#define MBEDTLS_ERROR_C

#endif /* MBEDTLS_CONFIG_H */
