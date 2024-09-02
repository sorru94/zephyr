/*
 * Copyright (c) 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_UUID_H_
#define ZEPHYR_INCLUDE_SYS_UUID_H_

/**
 * @file
 *
 * @brief Utility functions for the generation and parsing of Universal Unique Identifier.
 * @details This driver is compliant with RFC9562: https://datatracker.ietf.org/doc/rfc9562/
 */

#include <zephyr/kernel.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup uuid UUID
 * @ingroup utilities
 * @{
 */

/** @brief Number of bytes in the binary representation of a UUID. */
#define UUID_SIZE 16

/** @brief Length of the UUID canonical string representation. */
#define UUID_STR_LEN 36

/** @brief Length of the UUID base64 string representation. */
#define UUID_BASE64_LEN 24

/** @brief Length of the UUID base64 URL and filename safe string representation. */
#define UUID_BASE64URL_LEN 22

/** @brief Binary representation of a UUID. */
typedef uint8_t uuid_t[UUID_SIZE];

/**
 * @brief Generate a UUIDv4.
 *
 * @param out The UUID where the result will be written.
 */
void uuid_generate_v4(uuid_t out);

/**
 * @brief Generate a UUIDv5.
 *
 * @details This function computes a deterministic UUID starting from a namespace UUID and binary
 * data.
 *
 * @param[in] namespace The string representation of an UUID to be used as namespace.
 * @param[in] data A pointer to the data that will be hashed to produce the UUID.
 * @param[in] data_size The size of the data buffer.
 * @param[out] out The UUID where the result will be written.
 * @retval 0 if the UUID has been correctly generated and stored in @p out
 * @retval -EINVAL if @p input is not acceptable
 * @retval -ENOMEM if memory allocation failed
 * @retval -ENOTSUP if mbedTLS returned an unrecognized error
 */
int uuid_generate_v5(const uuid_t namespace, const void *data, size_t data_size, uuid_t out);

/**
 * @brief Parse a UUID from its canonical (RFC9562) string representation.
 *
 * @param input A pointer to the string to be parsed.
 * @param out The UUID where the result will be written.
 * @retval 0 if the UUID has been correctly parsed and stored in @p out
 * @retval -EINVAL if @p input is not acceptable
 */
int uuid_from_string(const char *input, uuid_t out);

/**
 * @brief Convert a UUID to its canonical (RFC9562) string representation.
 *
 * @param uuid The UUID to convert to string.
 * @param out A pointer to a previously allocated buffer where the result will be written.
 * @param out_size The size of the out buffer. Should be at least UUID_STR_LEN + 1 chars.
 * @retval 0 if the UUID has been converted and written in @p out
 * @retval -EINVAL if @p out is null or @p out_size is too small
 */
int uuid_to_string(const uuid_t uuid, char *out, size_t out_size);

/**
 * @brief Convert a UUID to its base 64 (RFC 3548, RFC 4648) string representation.
 *
 * @param uuid The UUID to convert to string.
 * @param out A pointer to a previously allocated buffer where the result will be written.
 * @param out_size The size of the out buffer. Should be at least UUID_BASE64_LEN + 1 chars.
 * @retval 0 if the UUID has been converted and written in @p out
 * @retval -EINVAL if @p out is null or @p out_size is too small
 */
int uuid_to_base64(const uuid_t uuid, char *out, size_t out_size);

/**
 * @brief Convert a UUID to its base 64 (RFC 4648 sec. 5) URL and filename safe string
 * representation.
 *
 * @param uuid The UUID to convert to string.
 * @param out A pointer to a previously allocated buffer where the result will be written.
 * @param out_size The size of the out buffer. Should be at least UUID_BASE64URL_LEN + 1 chars.
 * @retval 0 if the UUID has been converted and written in @p out
 * @retval -EINVAL if @p out is null or @p out_size is too small
 */
int uuid_to_base64url(const uuid_t uuid, char *out, size_t out_size);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_UUID_H_ */
