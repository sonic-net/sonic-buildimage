#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <float.h>

/*
 * We test the security invariant that buffer reads/writes never exceed
 * the declared buffer length when cJSON formats numbers.
 *
 * The vulnerable code uses sprintf() without bounds checking into a
 * fixed-size number_buffer. We simulate the same logic here and verify
 * that the output never exceeds the expected buffer size.
 *
 * Buffer size used in cJSON for number formatting is 64 bytes.
 */

#define CJSON_NUMBER_BUFFER_SIZE 64

/* Replicate the cJSON number-to-string logic under test */
static int format_number_to_buffer(double d, char *number_buffer, size_t buf_size)
{
    int length = 0;
    size_t i = 0;
    double test = 0.0;
    int had_zero = 0;

    if (isnan(d) || isinf(d)) {
        length = snprintf(number_buffer, buf_size, "null");
    } else {
        /* Use snprintf as a safe stand-in to measure what sprintf would write */
        length = snprintf(number_buffer, buf_size, "%1.15g", d);
        if (length < 0 || (size_t)length >= buf_size) {
            return -1; /* overflow detected */
        }

        /* Check if the result can be parsed back */
        if ((sscanf(number_buffer, "%lf", &test) != 1) || (test != d)) {
            length = snprintf(number_buffer, buf_size, "%1.17g", d);
            if (length < 0 || (size_t)length >= buf_size) {
                return -1; /* overflow detected */
            }
        }
    }

    /* Verify the string is null-terminated within bounds */
    for (i = 0; i < buf_size; i++) {
        if (number_buffer[i] == '\0') {
            had_zero = 1;
            break;
        }
    }

    if (!had_zero) {
        return -1; /* no null terminator found within buffer */
    }

    return length;
}

START_TEST(test_number_buffer_no_overflow)
{
    /* Invariant: formatted number output must never exceed the declared buffer size (64 bytes) */

    /* Adversarial double values that could produce long string representations */
    double payloads[] = {
        /* Normal values */
        0.0,
        1.0,
        -1.0,
        /* Very large values */
        DBL_MAX,
        -DBL_MAX,
        /* Very small values */
        DBL_MIN,
        -DBL_MIN,
        /* Subnormal values */
        5e-324,
        -5e-324,
        /* Special values */
        INFINITY,
        -INFINITY,
        NAN,
        /* Values that produce long decimal representations */
        1.0 / 3.0,
        2.0 / 3.0,
        1.0 / 7.0,
        1.0 / 9.0,
        /* Values near representational boundaries */
        1.7976931348623157e+308,
        -1.7976931348623157e+308,
        2.2250738585072014e-308,
        /* Values with many significant digits */
        1.2345678901234567890,
        9.9999999999999999999,
        1.0000000000000002,
        /* Negative zero */
        -0.0,
        /* Powers of 2 near overflow */
        (double)(1ULL << 53),
        -(double)(1ULL << 53),
        /* Values that stress %g formatting */
        1e100,
        1e-100,
        1e200,
        1e-200,
        1e300,
        1e-300,
        /* Crafted to maximize output length with %1.17g */
        0.10000000000000001,
        0.20000000000000001,
        0.30000000000000004,
        1.0000000000000004,
        /* Integer-like doubles */
        1234567890.0,
        12345678901234.0,
        /* Values near 1 */
        1.0 - DBL_EPSILON,
        1.0 + DBL_EPSILON,
        /* Hex-representable adversarial values */
        0x1.fffffffffffffp+1023,
        0x1.0000000000001p-1022,
        /* Repeated digit patterns */
        1.1111111111111111,
        2.2222222222222222,
        3.3333333333333333,
        /* Large negative exponent */
        1.23456789012345678e-300,
        /* Large positive exponent */
        1.23456789012345678e+300,
    };

    int num_payloads = (int)(sizeof(payloads) / sizeof(payloads[0]));

    for (int i = 0; i < num_payloads; i++) {
        /* Use a canary-protected buffer to detect overflows */
        char canary_buffer[CJSON_NUMBER_BUFFER_SIZE + 32];
        char *number_buffer = canary_buffer + 16; /* offset to detect underflow too */
        const char canary_byte = (char)0xAB;

        /* Initialize canary regions */
        memset(canary_buffer, canary_byte, sizeof(canary_buffer));

        int length = format_number_to_buffer(payloads[i], number_buffer, CJSON_NUMBER_BUFFER_SIZE);

        /* Invariant 1: function must not return overflow indicator */
        ck_assert_msg(length >= 0,
            "Payload[%d] = %g caused buffer overflow (length=%d)", i, payloads[i], length);

        /* Invariant 2: length must be within buffer bounds */
        ck_assert_msg(length < (int)CJSON_NUMBER_BUFFER_SIZE,
            "Payload[%d] = %g produced length %d >= buffer size %d",
            i, payloads[i], length, (int)CJSON_NUMBER_BUFFER_SIZE);

        /* Invariant 3: canary bytes after the buffer must be intact */
        for (int j = 0; j < 16; j++) {
            ck_assert_msg((unsigned char)canary_buffer[16 + CJSON_NUMBER_BUFFER_SIZE + j] == (unsigned char)canary_byte,
                "Payload[%d] = %g corrupted canary byte at offset %d after buffer",
                i, payloads[i], j);
        }

        /* Invariant 4: canary bytes before the buffer must be intact */
        for (int j = 0; j < 16; j++) {
            ck_assert_msg((unsigned char)canary_buffer[j] == (unsigned char)canary_byte,
                "Payload[%d] = %g corrupted canary byte at offset %d before buffer",
                i, payloads[i], j);
        }

        /* Invariant 5: the string must be null-terminated within the buffer */
        int found_null = 0;
        for (int j = 0; j < (int)CJSON_NUMBER_BUFFER_SIZE; j++) {
            if (number_buffer[j] == '\0') {
                found_null = 1;
                break;
            }
        }
        ck_assert_msg(found_null,
            "Payload[%d] = %g: no null terminator found within buffer bounds",
            i, payloads[i]);

        /* Invariant 6: reported length must match actual string length */
        if (length >= 0) {
            size_t actual_len = strnlen(number_buffer, CJSON_NUMBER_BUFFER_SIZE);
            ck_assert_msg((size_t)length == actual_len,
                "Payload[%d] = %g: reported length %d != actual string length %zu",
                i, payloads[i], length, actual_len);
        }
    }
}
END_TEST

START_TEST(test_number_buffer_sprintf_measurement)
{
    /*
     * Invariant: The maximum output of sprintf("%1.17g", d) for any finite double
     * must fit within the 64-byte number_buffer used by cJSON.
     *
     * We measure what sprintf WOULD write using snprintf with a large buffer,
     * then assert it fits within 64 bytes.
     */
    double adversarial[] = {
        DBL_MAX,
        -DBL_MAX,
        DBL_MIN,
        1.7976931348623157e+308,
        -1.7976931348623157e+308,
        2.2250738585072014e-308,
        1.0 / 3.0,
        1.0 / 7.0,
        0.1,
        0.2,
        0.3,
        1.23456789012345678901234567890e+300,
        1.23456789012345678901234567890e-300,
        9.99999999999999999e+307,
        1.00000000000000001e-307,
        /* Stress test: values known to produce long %g output */
        5e-324,
        1.7976931348623158e+308,
        2.225073858507201e-308,
        1.1102230246251565e-16,
        2.220446049250313e-16,
    };

    int num_adversarial = (int)(sizeof(adversarial) / sizeof(adversarial[0]));
    char large_buf[256];

    for (int i = 0; i < num_adversarial; i++) {
        int len15 = snprintf(large_buf, sizeof(large_buf), "%1.15g", adversarial[i]);
        int len17 = snprintf(large_buf, sizeof(large_buf), "%1.17g", adversarial[i]);

        /* Invariant: both format strings must produce output fitting in 64 bytes */
        ck_assert_msg(len15 >= 0 && len15 < (int)CJSON_NUMBER_BUFFER_SIZE,
            "%%1.15g for value[%d]=%g produces length %d >= buffer size %d",
            i, adversarial[i], len15, (int)CJSON_NUMBER_BUFFER_SIZE);

        ck_assert_msg(len17 >= 0 && len17 < (int)CJSON_NUMBER_BUFFER_SIZE,
            "%%1.17g for value[%d]=%g produces length %d >= buffer size %d",
            i, adversarial[i], len17, (int)CJSON_NUMBER_BUFFER_SIZE);
    }
}
END_TEST

START_TEST(test_null_nan_inf_formatting)
{
    /*
     * Invariant: NaN and Inf values must be formatted as "null" (4 bytes + NUL),
     * which always fits in the buffer.
     */
    double special_values[] = {
        INFINITY,
        -INFINITY,
        NAN,
        -NAN,
        /* Construct NaN with payload */
        __builtin_nan("0x7ff8000000000000"),
        __builtin_nan("0xffffffffffffffff"),
    };

    int num_special = (int)(sizeof(special_values) / sizeof(special_values[0]));

    for (int i = 0; i < num_special; i++) {
        char buf[CJSON_NUMBER_BUFFER_SIZE];
        memset(buf, 0xCC, sizeof(buf));

        int length = 0;
        if (isnan(special_values[i]) || isinf(special_values[i])) {
            length = snprintf(buf, sizeof(buf), "null");
        }

        /* Invariant: "null" is 4 chars, always fits */
        ck_assert_msg(length == 4,
            "Special value[%d] formatted to length %d, expected 4", i, length);

        ck_assert_msg(strncmp(buf, "null", sizeof(buf)) == 0,
            "Special value[%d] not formatted as 'null'", i);

        /* Invariant: buffer not overrun */
        ck_assert_msg(buf[4] == '\0',
            "Special value[%d]: null terminator missing at position 4", i);
    }
}
END_TEST

Suite *security_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Security_CWE120_BufferOverflow");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_number_buffer_no_overflow);
    tcase_add_test(tc_core, test_number_buffer_sprintf_measurement);
    tcase_add_test(tc_core, test_null_nan_inf_formatting);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = security_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}