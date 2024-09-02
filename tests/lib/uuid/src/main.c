/*
 * Copyright (c) 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/ztest.h>
#include <zephyr/sys/uuid.h>

ZTEST(uuid, test_uuid_v4)
{
	uuid_t uuid_1 = {0};
	uuid_t uuid_2 = {0};

	uuid_generate_v4(uuid_1);
	/* Check version and variant fields */
	zassert_equal(uuid_1[6U] >> 4U, 4U, "Generated UUID v4 contains an incorrect 'ver' field");
	zassert_equal(uuid_1[8U] >> 6U, 2U, "Generated UUID v4 contains an incorrect 'var' field");
	/* Check another generated UUID is different */
	uuid_generate_v4(uuid_2);
	zassert(memcmp(uuid_1, uuid_2, UUID_SIZE) != 0, "The two generated UUID v4 are equal.");
}

ZTEST(uuid, test_uuid_v5)
{
	uuid_t namespace;
	int result;
	uuid_t uuid;
	char uuid_str[UUID_STR_LEN + 1];

	result = uuid_from_string("6ba7b810-9dad-11d1-80b4-00c04fd430c8", namespace);
	zassert_true(result == 0, "uuid_from_string returned an error");
	result = uuid_generate_v5(namespace, "www.example.com", 15, uuid);
	zassert_true(result == 0, "uuid_generate_v5 returned an error");
	result = uuid_to_string(uuid, uuid_str, UUID_STR_LEN + 1);
	zassert_true(result == 0, "uuid_from_string returned an error");
	zassert_true(strcmp("2ed6657d-e927-568b-95e1-2665a8aea6a2", uuid_str) == 0,
		     "uuid_str != 2ed6657d-e927-568b-95e1-2665a8aea6a2");
}

ZTEST(uuid, test_uuid_from_string)
{
	const char *first_uuid_v4_string = "44b35f73-cfbd-43b4-8fef-ca7baea1375f";
	const char *second_uuid_v4_string = "6f2fd4cb-94a0-41c7-8d27-864c6b13b8c0";
	const char *third_uuid_v4_string = "8f65dbbc-5868-4015-8523-891cc0bffa58";
	const char *first_uuid_v5_string = "0575a569-51eb-575c-afe4-ce7fc03bcdc5";

	uuid_t first_uuid_v4;
	uuid_t second_uuid_v4;
	uuid_t third_uuid_v4;
	uuid_t first_uuid_v5;

	uint8_t expected_first_uuid_v4_byte_array[16u] = {0x44, 0xb3, 0x5f, 0x73, 0xcf, 0xbd,
							  0x43, 0xb4, 0x8f, 0xef, 0xca, 0x7b,
							  0xae, 0xa1, 0x37, 0x5f};
	uint8_t expected_second_uuid_v4_byte_array[16u] = {0x6f, 0x2f, 0xd4, 0xcb, 0x94, 0xa0,
							   0x41, 0xc7, 0x8d, 0x27, 0x86, 0x4c,
							   0x6b, 0x13, 0xb8, 0xc0};
	uint8_t expected_third_uuid_v4_byte_array[16u] = {0x8f, 0x65, 0xdb, 0xbc, 0x58, 0x68,
							  0x40, 0x15, 0x85, 0x23, 0x89, 0x1c,
							  0xc0, 0xbf, 0xfa, 0x58};
	uint8_t expected_first_uuid_v5_byte_array[16u] = {0x05, 0x75, 0xa5, 0x69, 0x51, 0xeb,
							  0x57, 0x5c, 0xaf, 0xe4, 0xce, 0x7f,
							  0xc0, 0x3b, 0xcd, 0xc5};

	int res = 0;

	res = uuid_from_string(first_uuid_v4_string, first_uuid_v4);
	zassert_equal(0, res, "uuid_from_string returned an error");
	for (size_t i = 0; i < 16; i++) {
		zassert_equal(expected_first_uuid_v4_byte_array[i], first_uuid_v4[i],
			      "first_uuid != from expected value");
	}

	res = uuid_from_string(second_uuid_v4_string, second_uuid_v4);
	zassert_equal(0, res, "uuid_from_string returned an error");
	for (size_t i = 0; i < 16; i++) {
		zassert_equal(expected_second_uuid_v4_byte_array[i], second_uuid_v4[i],
			      "second_uuid != from expected value");
	}

	res = uuid_from_string(third_uuid_v4_string, third_uuid_v4);
	zassert_equal(0, res, "uuid_from_string returned an error");
	for (size_t i = 0; i < 16; i++) {
		zassert_equal(expected_third_uuid_v4_byte_array[i], third_uuid_v4[i],
			      "third_uuid != from expected value");
	}

	res = uuid_from_string(first_uuid_v5_string, first_uuid_v5);
	zassert_equal(0, res, "uuid_from_string returned an error");
	for (size_t i = 0; i < 16; i++) {
		zassert_equal(expected_first_uuid_v5_byte_array[i], first_uuid_v5[i],
			      "uuid_v5!= from expected value");
	}
}

ZTEST(uuid, test_uuid_from_string_errors)
{
	const char *uuid_string_too_short = "44b35f73-cfbd-43b4-8fef-ca7baea1375";
	const char *uuid_string_too_long = "44b35f73-cfbd-43b4-8fef-ca7baea1375f0";
	const char *uuid_string_missing_hyphen = "44b35f73-cfbd-43b4-8fef0ca7baea1375f";
	const char *uuid_string_non_hex_digit = "44b35f73-cfLd-43b4-8fef-ca7baea1375f";

	int res;
	uuid_t uuid;

	res = uuid_from_string(uuid_string_too_short, uuid);
	zassert_equal(-EINVAL, res, "uuid_from_string returned incorrect error");

	res = uuid_from_string(uuid_string_too_long, uuid);
	zassert_equal(-EINVAL, res, "uuid_from_string returned incorrect error");

	res = uuid_from_string(uuid_string_missing_hyphen, uuid);
	zassert_equal(-EINVAL, res, "uuid_from_string returned incorrect error");

	res = uuid_from_string(uuid_string_non_hex_digit, uuid);
	zassert_equal(-EINVAL, res, "uuid_from_string returned incorrect error");
}

ZTEST(uuid, test_uuid_to_string)
{
	const uuid_t first_uuid_v4 = {0x44, 0xb3, 0x5f, 0x73, 0xcf, 0xbd, 0x43, 0xb4,
				      0x8f, 0xef, 0xca, 0x7b, 0xae, 0xa1, 0x37, 0x5f};
	const uuid_t second_uuid_v4 = {0x6f, 0x2f, 0xd4, 0xcb, 0x94, 0xa0, 0x41, 0xc7,
				       0x8d, 0x27, 0x86, 0x4c, 0x6b, 0x13, 0xb8, 0xc0};
	const uuid_t first_uuid_v5 = {0x05, 0x75, 0xa5, 0x69, 0x51, 0xeb, 0x57, 0x5c,
				      0xaf, 0xe4, 0xce, 0x7f, 0xc0, 0x3b, 0xcd, 0xc5};

	char first_uuid_v4_string[UUID_STR_LEN + 1];
	char second_uuid_v4_string[UUID_STR_LEN + 1];
	char first_uuid_v5_string[UUID_STR_LEN + 1];

	const char *expected_first_uuid_v4_string = "44b35f73-cfbd-43b4-8fef-ca7baea1375f";
	const char *expected_second_uuid_v4_string = "6f2fd4cb-94a0-41c7-8d27-864c6b13b8c0";
	const char *expected_first_uuid_v5_string = "0575a569-51eb-575c-afe4-ce7fc03bcdc5";

	int err = 0;

	err = uuid_to_string(first_uuid_v4, first_uuid_v4_string, UUID_STR_LEN + 1);
	zassert_equal(0, err, "uuid_to_string returned an error");
	zassert_true(strcmp(expected_first_uuid_v4_string, first_uuid_v4_string) == 0,
		     "expected %s", expected_first_uuid_v4_string);

	err = uuid_to_string(second_uuid_v4, second_uuid_v4_string, UUID_STR_LEN + 1);
	zassert_equal(0, err, "uuid_to_string returned an error");
	zassert_true(strcmp(expected_second_uuid_v4_string, second_uuid_v4_string) == 0,
		     "expected %s", expected_second_uuid_v4_string);

	err = uuid_to_string(first_uuid_v5, first_uuid_v5_string, UUID_STR_LEN + 1);
	zassert_equal(0, err, "uuid_to_string returned an error");
	zassert_true(strcmp(expected_first_uuid_v5_string, first_uuid_v5_string) == 0,
		     "expected %s", expected_first_uuid_v5_string);
}

ZTEST(uuid, test_uuid_to_base64)
{
	char first_uuid_v4_base64[UUID_BASE64_LEN + 1] = {0};
	char second_uuid_v4_base64[UUID_BASE64_LEN + 1] = {0};
	char first_uuid_v5_base64[UUID_BASE64_LEN + 1] = {0};

	const uuid_t first_uuid_v4 = {0x44, 0xb3, 0x5f, 0x73, 0xcf, 0xbd, 0x43, 0xb4,
				      0x8f, 0xef, 0xca, 0x7b, 0xae, 0xa1, 0x37, 0x5f};
	const uuid_t second_uuid_v4 = {0x6f, 0x2f, 0xd4, 0xcb, 0x94, 0xa0, 0x41, 0xc7,
				       0x8d, 0x27, 0x86, 0x4c, 0x6b, 0x13, 0xb8, 0xc0};
	const uuid_t first_uuid_v5 = {0x05, 0x75, 0xa5, 0x69, 0x51, 0xeb, 0x57, 0x5c,
				      0xaf, 0xe4, 0xce, 0x7f, 0xc0, 0x3b, 0xcd, 0xc5};

	char expected_first_uuid_v4_base64[] = "RLNfc8+9Q7SP78p7rqE3Xw==";
	char expected_second_uuid_v4_base64[] = "by/Uy5SgQceNJ4ZMaxO4wA==";
	char expected_first_uuid_v5_base64[] = "BXWlaVHrV1yv5M5/wDvNxQ==";

	int err = 0;

	err = uuid_to_base64(first_uuid_v4, first_uuid_v4_base64, UUID_BASE64_LEN + 1);
	zassert_equal(0, err, "uuid_to_base64 returned an error");
	zassert_true(strcmp(expected_first_uuid_v4_base64, first_uuid_v4_base64) == 0,
		     "expected: '%s', gotten: '%s'", expected_first_uuid_v4_base64,
		     first_uuid_v4_base64);

	err = uuid_to_base64(second_uuid_v4, second_uuid_v4_base64, UUID_BASE64_LEN + 1);
	zassert_equal(0, err, "uuid_to_base64 returned an error");
	zassert_true(strcmp(expected_second_uuid_v4_base64, second_uuid_v4_base64) == 0,
		     "expected: '%s', gotten: '%s'", expected_second_uuid_v4_base64,
		     second_uuid_v4_base64);

	err = uuid_to_base64(first_uuid_v5, first_uuid_v5_base64, UUID_BASE64_LEN + 1);
	zassert_equal(0, err, "uuid_to_base64 returned an error");
	zassert_true(strcmp(expected_first_uuid_v5_base64, first_uuid_v5_base64) == 0,
		     "expected: '%s', gotten: '%s'", expected_first_uuid_v5_base64,
		     first_uuid_v5_base64);
}

ZTEST(uuid, test_uuid_to_base64url)
{
	char first_uuid_v4_base64url[UUID_BASE64URL_LEN + 1] = {0};
	char second_uuid_v4_base64url[UUID_BASE64URL_LEN + 1] = {0};
	char first_uuid_v5_base64url[UUID_BASE64URL_LEN + 1] = {0};

	const uuid_t first_uuid_v4 = {0x44, 0xb3, 0x5f, 0x73, 0xcf, 0xbd, 0x43, 0xb4,
				      0x8f, 0xef, 0xca, 0x7b, 0xae, 0xa1, 0x37, 0x5f};
	const uuid_t second_uuid_v4 = {0x6f, 0x2f, 0xd4, 0xcb, 0x94, 0xa0, 0x41, 0xc7,
				       0x8d, 0x27, 0x86, 0x4c, 0x6b, 0x13, 0xb8, 0xc0};
	const uuid_t first_uuid_v5 = {0x05, 0x75, 0xa5, 0x69, 0x51, 0xeb, 0x57, 0x5c,
				      0xaf, 0xe4, 0xce, 0x7f, 0xc0, 0x3b, 0xcd, 0xc5};

	char expected_first_uuid_v4_base64url[] = "RLNfc8-9Q7SP78p7rqE3Xw";
	char expected_second_uuid_v4_base64url[] = "by_Uy5SgQceNJ4ZMaxO4wA";
	char expected_first_uuid_v5_base64url[] = "BXWlaVHrV1yv5M5_wDvNxQ";

	int err = 0;

	err = uuid_to_base64url(first_uuid_v4, first_uuid_v4_base64url, UUID_BASE64URL_LEN + 1);
	zassert_equal(0, err, "uuid_to_base64url returned an error");
	zassert_true(strcmp(expected_first_uuid_v4_base64url, first_uuid_v4_base64url) == 0,
		     "expected: '%s', gotten: '%s'", expected_first_uuid_v4_base64url,
		     first_uuid_v4_base64url);

	err = uuid_to_base64url(second_uuid_v4, second_uuid_v4_base64url, UUID_BASE64URL_LEN + 1);
	zassert_equal(0, err, "uuid_to_base64url returned an error");
	zassert_true(strcmp(expected_second_uuid_v4_base64url, second_uuid_v4_base64url) == 0,
		     "expected: '%s', gotten: '%s'", expected_second_uuid_v4_base64url,
		     second_uuid_v4_base64url);

	err = uuid_to_base64url(first_uuid_v5, first_uuid_v5_base64url, UUID_BASE64URL_LEN + 1);
	zassert_equal(0, err, "uuid_to_base64url returned an error");
	zassert_true(strcmp(expected_first_uuid_v5_base64url, first_uuid_v5_base64url) == 0,
		     "expected: '%s', gotten: '%s'", expected_first_uuid_v5_base64url,
		     first_uuid_v5_base64url);
}

ZTEST_SUITE(uuid, NULL, NULL, NULL, NULL, NULL);
