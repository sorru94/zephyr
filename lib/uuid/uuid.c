/*
 * Copyright (c) 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/uuid.h>

#include <ctype.h>
#include <string.h>
#include <stdio.h>

#if defined(CONFIG_UUID_V4)
#include <zephyr/random/random.h>
#endif

#if defined(CONFIG_UUID_V5)
#include <mbedtls/md.h>
#endif

#if defined(CONFIG_UUID_BASE64)
#include <zephyr/sys/base64.h>
#endif

#define UUID_STR_POSITION_FIRST_HYPHEN  (8U)
#define UUID_STR_POSITION_SECOND_HYPHEN (13U)
#define UUID_STR_POSITION_THIRD_HYPHEN  (18U)
#define UUID_STR_POSITION_FOURTH_HYPHEN (23U)

#define UUID_POSITION_VERSION (6U)
#define UUID_OFFSET_VERSION   (4U)
#define UUID_MASK_VERSION     (0xF0U)
#define UUID_POSITION_VARIANT (8U)
#define UUID_OFFSET_VARIANT   (6U)
#define UUID_MASK_VARIANT     (0xC0)

#define UUID_V4_VERSION (4U)
#define UUID_V4_VARIANT (2U)
#define UUID_V5_VERSION (5U)
#define UUID_V5_VARIANT (2U)

#if defined(CONFIG_UUID_V4) || defined(CONFIG_UUID_V5)
static void overwrite_uuid_version_and_variant(uuid_t uuid, uint8_t version, uint8_t variant)
{
	/* Clear the 'ver' and 'var' fields */
	uuid[UUID_POSITION_VERSION] &= ~UUID_MASK_VERSION;
	uuid[UUID_POSITION_VARIANT] &= ~UUID_MASK_VARIANT;
	/* Update the 'ver' and 'var' fields */
	uuid[UUID_POSITION_VERSION] |= (uint8_t)(version << UUID_OFFSET_VERSION);
	uuid[UUID_POSITION_VARIANT] |= (uint8_t)(variant << UUID_OFFSET_VARIANT);
}
#endif

static bool should_be_hyphen(unsigned int position)
{
	switch (position) {
	case UUID_STR_POSITION_FIRST_HYPHEN:
	case UUID_STR_POSITION_SECOND_HYPHEN:
	case UUID_STR_POSITION_THIRD_HYPHEN:
	case UUID_STR_POSITION_FOURTH_HYPHEN:
		return true;
	default:
		return false;
	}
}

#if defined(CONFIG_UUID_V4)
int uuid_generate_v4(uuid_t out)
{
	if (out == NULL) {
		return -EINVAL;
	}
	/* Fill the whole UUID struct with a random number */
	sys_rand_get(out, UUID_SIZE);
	/* Update version and variant */
	overwrite_uuid_version_and_variant(out, UUID_V4_VERSION, UUID_V4_VARIANT);
	return 0;
}
#endif

#if defined(CONFIG_UUID_V5)
int uuid_generate_v5(const uuid_t namespace, const void *data, size_t data_size, uuid_t out)
{
	if ((out == NULL) || (namespace == NULL)) {
		return -EINVAL;
	}
	int mbedtls_err = 0;
	mbedtls_md_context_t ctx = {0};
	const mbedtls_md_info_t *md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA1);
	const size_t sha_1_bytes = 20;
	uint8_t sha_result[sha_1_bytes];

	mbedtls_md_init(&ctx);
	mbedtls_err = mbedtls_md_setup(&ctx, md_info, 0);
	/* Might return: MBEDTLS_ERR_MD_BAD_INPUT_DATA or MBEDTLS_ERR_MD_ALLOC_FAILED */
	switch (mbedtls_err) {
	case 0:
		break;
	case MBEDTLS_ERR_MD_BAD_INPUT_DATA:
		return -EINVAL;
	case MBEDTLS_ERR_MD_ALLOC_FAILED:
		return -ENOMEM;
	default:
		return -ENOTSUP;
	}
	mbedtls_err = mbedtls_md_starts(&ctx);
	if (mbedtls_err != 0) {
		/* Might return MBEDTLS_ERR_MD_BAD_INPUT_DATA */
		return -EINVAL;
	}
	mbedtls_err = mbedtls_md_update(&ctx, namespace, UUID_SIZE);
	if (mbedtls_err != 0) {
		/* Might return MBEDTLS_ERR_MD_BAD_INPUT_DATA */
		return -EINVAL;
	}
	mbedtls_err = mbedtls_md_update(&ctx, data, data_size);
	if (mbedtls_err != 0) {
		/* Might return MBEDTLS_ERR_MD_BAD_INPUT_DATA */
		return -EINVAL;
	}
	mbedtls_err = mbedtls_md_finish(&ctx, sha_result);
	if (mbedtls_err != 0) {
		/* Might return MBEDTLS_ERR_MD_BAD_INPUT_DATA */
		return -EINVAL;
	}
	mbedtls_md_free(&ctx);

	/* Store the computed SHA1 in the out struct */
	for (unsigned int i = 0; i < UUID_SIZE; i++) {
		out[i] = sha_result[i];
	}
	/* Update version and variant */
	overwrite_uuid_version_and_variant(out, UUID_V5_VERSION, UUID_V5_VARIANT);

	return 0;
}
#endif

int uuid_copy(uuid_t dst, const uuid_t src)
{
	if ((dst == NULL) || (src == NULL)) {
		return -EINVAL;
	}
	memcpy(dst, src, UUID_SIZE);
	return 0;
}

int uuid_from_buffer(const uint8_t data[UUID_SIZE], uuid_t out)
{
	if ((data == NULL) || (out == NULL)) {
		return -EINVAL;
	}
	memcpy(out, data, UUID_SIZE);
	return 0;
}

int uuid_from_string(const char input[UUID_STR_LEN], uuid_t out)
{
	if ((input == NULL) || (strlen(input) + 1 != UUID_STR_LEN) || (out == NULL)) {
		return -EINVAL;
	}
	for (unsigned int i = 0; i < UUID_STR_LEN - 1; i++) {
		char char_i = input[i];
		/* Check that hyphens are in the right place */
		if (should_be_hyphen(i)) {
			if (char_i != '-') {
				return -EINVAL;
			}
			continue;
		}
		/* Check if the given input is hexadecimal */
		if (!isxdigit(char_i)) {
			return -EINVAL;
		}
	}

	/* Content parsing */
	unsigned int input_idx = 0U;
	unsigned int out_idx = 0U;

	while (input_idx < UUID_STR_LEN - 1) {
		if (should_be_hyphen(input_idx)) {
			input_idx += 1;
			continue;
		}

		size_t hex2bin_rc =
			hex2bin(&input[input_idx], 2, &out[out_idx], UUID_SIZE - out_idx);
		if (hex2bin_rc != 1) {
			return -EINVAL;
		}
		out_idx++;
		input_idx += 2;
	}
	return 0;
}

int uuid_to_buffer(struct uuid input, uint8_t buff[UUID_SIZE])
{
	if (buff == NULL) {
		return -EINVAL;
	}
	memcpy(buff, input.val, UUID_SIZE);
	return 0;
}

int uuid_to_string(struct uuid input, char out[UUID_STR_LEN])
{
	if ((uuid == NULL) || out == NULL) {
		return -EINVAL;
	}
	snprintf(out, UUID_STR_LEN,
		 "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x", uuid[0],
		 uuid[1], uuid[2], uuid[3], uuid[4], uuid[5], uuid[6], uuid[7], uuid[8], uuid[9],
		 uuid[10], uuid[11], uuid[12], uuid[13], uuid[14], uuid[15]);
	return 0;
}

#if defined(CONFIG_UUID_BASE64)
int uuid_to_base64(const uuid_t uuid, char out[UUID_BASE64_LEN])
{
	if ((uuid == NULL) || out == NULL) {
		return -EINVAL;
	}

	size_t olen = 0;

	base64_encode(out, UUID_BASE64_LEN, &olen, uuid, UUID_SIZE);
	return 0;
}

int uuid_to_base64url(const uuid_t uuid, char out[UUID_BASE64URL_LEN])
{
	if ((uuid == NULL) || out == NULL) {
		return -EINVAL;
	}

	/* Convert UUID to RFC 3548/4648 base 64 notation */
	size_t olen = 0;
	char uuid_base64[UUID_BASE64_LEN] = {0};

	base64_encode(uuid_base64, UUID_BASE64_LEN, &olen, uuid, UUID_SIZE);
	/* Convert UUID to RFC 4648 sec. 5 URL and filename safe base 64 notation */
	for (unsigned int i = 0; i < UUID_BASE64URL_LEN - 1; i++) {
		if (uuid_base64[i] == '+') {
			uuid_base64[i] = '-';
		}
		if (uuid_base64[i] == '/') {
			uuid_base64[i] = '_';
		}
	}
	memcpy(out, uuid_base64, UUID_BASE64URL_LEN - 1);
	out[UUID_BASE64URL_LEN - 1] = 0;
	return 0;
}
#endif
